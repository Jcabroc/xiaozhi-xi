#pragma once

#include "led/led.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <mutex>

class XiBodyRgbStatusLed : public Led {
public:
    XiBodyRgbStatusLed(gpio_num_t red, gpio_num_t green, gpio_num_t blue, bool active_low);
    ~XiBodyRgbStatusLed() override;
    void OnStateChanged() override;

private:
    static void TimerCallback(void* context);
    void SetColor(bool red, bool green, bool blue);
    void Toggle();
    void Write(bool red, bool green, bool blue);

    gpio_num_t pins_[3];
    bool active_low_;
    std::mutex mutex_;
    bool desired_[3] = {};
    bool on_ = false;
    esp_timer_handle_t timer_ = nullptr;
};