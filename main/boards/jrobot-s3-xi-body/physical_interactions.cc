#include "physical_interactions.h"

#include <driver/touch_pad.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <sdkconfig.h>

#include <algorithm>
#include <cmath>

#include "config.h"

namespace {
constexpr char kTag[] = "XiPhysical";
constexpr touch_pad_t kTouchPads[] = {TOUCH_PAD_NUM1, TOUCH_PAD_NUM2};
constexpr uint32_t kTouchPollMs = 20;
constexpr uint32_t kImuPollMs = 50;
constexpr uint32_t kShortTouchMinMs = 35;
constexpr uint32_t kShortTouchMaxMs = 900;
constexpr uint32_t kTouchEventCooldownMs = 500;
constexpr uint8_t kTouchDebounceSamples = 3;
constexpr uint8_t kShakeDebounceSamples = 3;
constexpr uint8_t kShakeReleaseSamples = 5;
constexpr uint32_t kI2cTimeoutMs = 20;
constexpr float kAccelLsbPerG = 16384.0f;
constexpr float kGravityFilterAlpha = 0.05f;
constexpr float kMgPerG = 1000.0f;
constexpr uint32_t kTouchDelta = CONFIG_XI_BODY_TOUCH_DELTA;
constexpr uint32_t kShakeThresholdMg = CONFIG_XI_BODY_SHAKE_THRESHOLD_MG;
constexpr uint32_t kShakeReleaseMg = CONFIG_XI_BODY_SHAKE_RELEASE_MG;
constexpr uint32_t kShakeCooldownMs = CONFIG_XI_BODY_SHAKE_COOLDOWN_MS;
}  // namespace

const char* PhysicalInteractionEventName(PhysicalInteractionEvent event) {
    switch (event) {
        case PhysicalInteractionEvent::kTouch1Short: return "touch_1_short";
        case PhysicalInteractionEvent::kTouch2Short: return "touch_2_short";
        case PhysicalInteractionEvent::kShake: return "shake";
    }
    return "unknown";
}

bool PhysicalInteractions::Start(EventCallback callback) {
    if (task_ != nullptr || callback == nullptr || i2c_bus_ == nullptr) return false;
    callback_ = std::move(callback);
    const BaseType_t result = xTaskCreate(TaskEntry, "xi_physical", 4096, this, 2, &task_);
    if (result != pdPASS) {
        task_ = nullptr;
        callback_ = nullptr;
        return false;
    }
    return true;
}

void PhysicalInteractions::TaskEntry(void* context) {
    static_cast<PhysicalInteractions*>(context)->Run();
}

void PhysicalInteractions::Run() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    const bool touch_ready = InitializeTouch();
    imu_online_ = InitializeImu();
    ESP_LOGI(kTag, "Touch: %s; MPU-6050: %s",
             touch_ready ? "ready" : "unavailable", imu_online_ ? "ready" : "unavailable");

    uint32_t last_imu_ms = 0;
    while (true) {
        const uint32_t now_ms = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        if (touch_ready) PollTouch(now_ms);
        if (imu_online_ && now_ms - last_imu_ms >= kImuPollMs) {
            last_imu_ms = now_ms;
            PollImu(now_ms);
        }
        vTaskDelay(pdMS_TO_TICKS(kTouchPollMs));
    }
}

bool PhysicalInteractions::InitializeTouch() {
    esp_err_t result = touch_pad_init();
    if (result == ESP_OK) result = touch_pad_config(kTouchPads[0]);
    if (result == ESP_OK) result = touch_pad_config(kTouchPads[1]);
    if (result == ESP_OK) {
        result = touch_pad_set_channel_mask(BIT(TOUCH_PAD_NUM1) | BIT(TOUCH_PAD_NUM2));
    }
    if (result == ESP_OK) result = touch_pad_fsm_start();
    if (result != ESP_OK) {
        ESP_LOGE(kTag, "Touch initialization failed: %s", esp_err_to_name(result));
        return false;
    }

    uint64_t sums[2] = {};
    uint32_t counts[2] = {};
    for (int sample = 0; sample < 24; ++sample) {
        for (size_t index = 0; index < touch_.size(); ++index) {
            uint32_t raw = 0;
            if (touch_pad_read_raw_data(kTouchPads[index], &raw) == ESP_OK && raw != 0) {
                sums[index] += raw;
                ++counts[index];
            }
        }
        vTaskDelay(pdMS_TO_TICKS(kTouchPollMs));
    }
    for (size_t index = 0; index < touch_.size(); ++index) {
        if (counts[index] == 0) return false;
        touch_[index].baseline = static_cast<uint32_t>(sums[index] / counts[index]);
        ESP_LOGI(kTag, "T%u baseline=%lu delta=%lu", static_cast<unsigned>(index + 1),
                 static_cast<unsigned long>(touch_[index].baseline),
                 static_cast<unsigned long>(kTouchDelta));
    }
    return true;
}

bool PhysicalInteractions::InitializeImu() {
    if (i2c_bus_ == nullptr) return false;
    const i2c_device_config_t config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = IMU_I2C_ADDRESS,
        .scl_speed_hz = 100000,
    };
    esp_err_t result = i2c_master_bus_add_device(i2c_bus_, &config, &imu_);
    if (result != ESP_OK) {
        ESP_LOGW(kTag, "MPU-6050 device registration failed: %s", esp_err_to_name(result));
        return false;
    }

    const uint8_t wake[] = {0x6B, 0x00};
    const uint8_t filter[] = {0x1A, 0x03};  // 44 Hz low-pass filter
    const uint8_t sample_rate[] = {0x19, 0x09};  // 100 Hz internal sample rate
    const uint8_t accel_range[] = {0x1C, 0x00};  // +/-2 g
    for (const auto* command : {wake, filter, sample_rate, accel_range}) {
        result = i2c_master_transmit(imu_, command, 2, kI2cTimeoutMs);
        if (result != ESP_OK) {
            ESP_LOGW(kTag, "MPU-6050 setup failed: %s", esp_err_to_name(result));
            i2c_master_bus_rm_device(imu_);
            imu_ = nullptr;
            return false;
        }
    }
    return true;
}

void PhysicalInteractions::PollTouch(uint32_t now_ms) {
    for (size_t index = 0; index < touch_.size(); ++index) {
        TouchChannel& channel = touch_[index];
        uint32_t raw = 0;
        if (touch_pad_read_raw_data(kTouchPads[index], &raw) != ESP_OK || raw == 0) continue;

        const uint32_t threshold_delta = channel.active ? std::max<uint32_t>(1, kTouchDelta / 2) : kTouchDelta;
        const bool candidate = raw > channel.baseline + threshold_delta;
        if (candidate != channel.candidate) {
            channel.candidate = candidate;
            channel.stable_samples = 1;
        } else if (channel.stable_samples < kTouchDebounceSamples) {
            ++channel.stable_samples;
        }

        if (channel.candidate != channel.active && channel.stable_samples >= kTouchDebounceSamples) {
            channel.active = channel.candidate;
            if (channel.active) {
                channel.press_started_ms = now_ms;
                if (index == 1) {
                    ESP_LOGI(kTag, "T2 GPIO2 touched raw=%lu baseline=%lu delta=%lu",
                             static_cast<unsigned long>(raw),
                             static_cast<unsigned long>(channel.baseline),
                             static_cast<unsigned long>(raw - channel.baseline));
                }
            } else {
                const uint32_t duration = now_ms - channel.press_started_ms;
                if (index == 1) {
                    ESP_LOGI(kTag, "T2 GPIO2 released duration=%lu ms raw=%lu baseline=%lu",
                             static_cast<unsigned long>(duration),
                             static_cast<unsigned long>(raw),
                             static_cast<unsigned long>(channel.baseline));
                }
                if (duration >= kShortTouchMinMs && duration <= kShortTouchMaxMs &&
                    now_ms - channel.last_event_ms >= kTouchEventCooldownMs) {
                    channel.last_event_ms = now_ms;
                    if (index == 1) ESP_LOGI(kTag, "T2 short touch accepted; emitting touch_2_short");
                    Emit(index == 0 ? PhysicalInteractionEvent::kTouch1Short
                                    : PhysicalInteractionEvent::kTouch2Short);
                } else if (index == 1) {
                    ESP_LOGI(kTag, "T2 short touch not emitted: duration=%lu ms%s",
                             static_cast<unsigned long>(duration),
                             duration > kShortTouchMaxMs ? " exceeds 900 ms" :
                             duration < kShortTouchMinMs ? " below 35 ms" : " cooldown active");
                }
            }
        }

        if (!channel.active && !channel.candidate && raw > 0) {
            // Slow baseline tracking compensates for drift without following a touch transient.
            const int32_t error = static_cast<int32_t>(raw) - static_cast<int32_t>(channel.baseline);
            channel.baseline = static_cast<uint32_t>(static_cast<int32_t>(channel.baseline) + error / 512);
        }
    }
}

void PhysicalInteractions::PollImu(uint32_t now_ms) {
    uint8_t reg = 0x3B;
    uint8_t data[6] = {};
    const esp_err_t result = i2c_master_transmit_receive(imu_, &reg, 1, data, sizeof(data), kI2cTimeoutMs);
    if (result != ESP_OK) {
        if (imu_error_streak_ < 10) ++imu_error_streak_;
        if (imu_error_streak_ == 10) {
            ESP_LOGW(kTag, "MPU-6050 read errors; motion events paused");
        }
        return;
    }
    imu_error_streak_ = 0;

    float accel[3] = {
        static_cast<int16_t>((data[0] << 8) | data[1]) * kMgPerG / kAccelLsbPerG,
        static_cast<int16_t>((data[2] << 8) | data[3]) * kMgPerG / kAccelLsbPerG,
        static_cast<int16_t>((data[4] << 8) | data[5]) * kMgPerG / kAccelLsbPerG,
    };
    if (!gravity_initialized_) {
        for (size_t axis = 0; axis < gravity_mg_.size(); ++axis) gravity_mg_[axis] = accel[axis];
        gravity_initialized_ = true;
        return;
    }

    float dynamic_squared = 0.0f;
    for (size_t axis = 0; axis < gravity_mg_.size(); ++axis) {
        const float dynamic = accel[axis] - gravity_mg_[axis];
        dynamic_squared += dynamic * dynamic;
        gravity_mg_[axis] += kGravityFilterAlpha * dynamic;
    }
    const float dynamic_mg = std::sqrt(dynamic_squared);

    if (shake_armed_) {
        shake_high_samples_ = dynamic_mg >= kShakeThresholdMg ? shake_high_samples_ + 1 : 0;
        if (shake_high_samples_ >= kShakeDebounceSamples && now_ms - last_shake_ms_ >= kShakeCooldownMs) {
            shake_armed_ = false;
            shake_high_samples_ = 0;
            shake_low_samples_ = 0;
            last_shake_ms_ = now_ms;
            Emit(PhysicalInteractionEvent::kShake);
        }
    } else {
        shake_low_samples_ = dynamic_mg <= kShakeReleaseMg ? shake_low_samples_ + 1 : 0;
        if (shake_low_samples_ >= kShakeReleaseSamples && now_ms - last_shake_ms_ >= kShakeCooldownMs) {
            shake_armed_ = true;
            shake_low_samples_ = 0;
        }
    }
}

void PhysicalInteractions::Emit(PhysicalInteractionEvent event) {
    if (callback_) callback_(event);
}
