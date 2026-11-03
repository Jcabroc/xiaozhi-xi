#pragma once

#include <driver/i2c_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <array>
#include <cstdint>
#include <functional>

enum class PhysicalInteractionEvent : uint8_t {
    kTouch1Short,
    kTouch2Short,
    kShake,
};

const char* PhysicalInteractionEventName(PhysicalInteractionEvent event);

class PhysicalInteractions {
public:
    using EventCallback = std::function<void(PhysicalInteractionEvent)>;

    explicit PhysicalInteractions(i2c_master_bus_handle_t i2c_bus) : i2c_bus_(i2c_bus) {}
    bool Start(EventCallback callback);

private:
    struct TouchChannel {
        uint32_t baseline = 0;
        uint32_t press_started_ms = 0;
        uint32_t last_event_ms = 0;
        uint8_t stable_samples = 0;
        bool active = false;
        bool candidate = false;
    };

    static void TaskEntry(void* context);
    void Run();
    bool InitializeTouch();
    bool InitializeImu();
    void PollTouch(uint32_t now_ms);
    void PollImu(uint32_t now_ms);
    void Emit(PhysicalInteractionEvent event);

    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    i2c_master_dev_handle_t imu_ = nullptr;
    EventCallback callback_;
    std::array<TouchChannel, 2> touch_{};
    std::array<float, 3> gravity_mg_{};
    bool gravity_initialized_ = false;
    bool shake_armed_ = true;
    uint8_t shake_high_samples_ = 0;
    uint8_t shake_low_samples_ = 0;
    uint32_t last_shake_ms_ = 0;
    uint8_t imu_error_streak_ = 0;
    bool imu_online_ = false;
    TaskHandle_t task_ = nullptr;
};
