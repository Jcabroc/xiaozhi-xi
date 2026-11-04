#include "rgb_status_led.h"

#include <esp_err.h>

#include "application.h"

XiBodyRgbStatusLed::XiBodyRgbStatusLed(gpio_num_t red, gpio_num_t green, gpio_num_t blue, bool active_low)
    : pins_{red, green, blue}, active_low_(active_low) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << red) | (1ULL << green) | (1ULL << blue),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&config));
    Write(false, false, false);

    esp_timer_create_args_t args = {
        .callback = TimerCallback,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "xi_rgb_status",
        .skip_unhandled_events = true,
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &timer_));
}

XiBodyRgbStatusLed::~XiBodyRgbStatusLed() {
    if (timer_ != nullptr) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

void XiBodyRgbStatusLed::Write(bool red, bool green, bool blue) {
    const bool values[] = {red, green, blue};
    for (int i = 0; i < 3; ++i) {
        gpio_set_level(pins_[i], values[i] != active_low_ ? 1 : 0);
    }
}

void XiBodyRgbStatusLed::SetColor(bool red, bool green, bool blue) {
    esp_timer_stop(timer_);
    std::lock_guard<std::mutex> lock(mutex_);
    desired_[0] = red;
    desired_[1] = green;
    desired_[2] = blue;
    on_ = true;
    Write(red, green, blue);
}

void XiBodyRgbStatusLed::TimerCallback(void* context) {
    static_cast<XiBodyRgbStatusLed*>(context)->Toggle();
}

void XiBodyRgbStatusLed::Toggle() {
    std::lock_guard<std::mutex> lock(mutex_);
    on_ = !on_;
    Write(on_ && desired_[0], on_ && desired_[1], on_ && desired_[2]);
}

void XiBodyRgbStatusLed::OnStateChanged() {
    auto state = Application::GetInstance().GetDeviceState();
    switch (state) {
        case kDeviceStateStarting:
        case kDeviceStateConnecting:
        case kDeviceStateActivating:
            SetColor(false, false, true);
            esp_timer_start_periodic(timer_, state == kDeviceStateStarting ? 250000 : 400000);
            break;
        case kDeviceStateWifiConfiguring:
        case kDeviceStateUpgrading:
            SetColor(true, true, false);
            esp_timer_start_periodic(timer_, state == kDeviceStateUpgrading ? 250000 : 500000);
            break;
        case kDeviceStateIdle:
            SetColor(false, true, false);
            break;
        case kDeviceStateListening:
        case kDeviceStateAudioTesting:
            SetColor(false, true, true);
            break;
        case kDeviceStateSpeaking:
            SetColor(true, false, true);
            break;
        case kDeviceStateFatalError:
            SetColor(true, false, false);
            esp_timer_start_periodic(timer_, 200000);
            break;
        default:
            SetColor(false, false, false);
            break;
    }
}