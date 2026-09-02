#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_log.h>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/display.h"
#include "led/single_led.h"
#include "wifi_board.h"
#include "xi_body_peripherals.h"

#define TAG "XiBodyV2"

class JRobotS3XiBody : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    Display* display_ = nullptr;
    Button boot_button_;

    void InitializeI2cReservation() {
        i2c_master_bus_config_t config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = XI_BODY_I2C_SDA_GPIO,
            .scl_io_num = XI_BODY_I2C_SCL_GPIO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {.enable_internal_pullup = 1},
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&config, &i2c_bus_));
        ESP_LOGI(TAG, "I2C reserved for PCA9685 (0x%02X) and IMU (0x%02X)",
                 PCA9685_I2C_ADDRESS, IMU_I2C_ADDRESS);
    }

    void InitializeTftReservation() {
        spi_bus_config_t config = {};
        config.mosi_io_num = TFT_MOSI_GPIO;
        config.miso_io_num = GPIO_NUM_NC;
        config.sclk_io_num = TFT_SCK_GPIO;
        config.quadwp_io_num = GPIO_NUM_NC;
        config.quadhd_io_num = GPIO_NUM_NC;
        config.max_transfer_sz = TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &config, SPI_DMA_CH_AUTO));
        ESP_LOGW(TAG, "128x160 TFT controller pending confirmation; display driver is disabled");
        display_ = new NoDisplay();
    }

    void InitializeInputReservations() {
        gpio_config_t pir_config = {
            .pin_bit_mask = 1ULL << PIR_IN_GPIO,
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        ESP_ERROR_CHECK(gpio_config(&pir_config));
        // Touch and battery are deliberately data-only reservations until their hardware is validated.
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

public:
    JRobotS3XiBody() : boot_button_(BOOT_BUTTON_GPIO) {
        InitializeI2cReservation();
        InitializeTftReservation();
        InitializeInputReservations();
        InitializeButtons();
    }

    AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT,
            AUDIO_I2S_MIC_GPIO_DIN);
#endif
        static const bool input_gain_configured = (codec.SetInputGain(0.5f), true);
        (void)input_gain_configured;
        return &codec;
    }

    Display* GetDisplay() override { return display_; }

    Led* GetLed() override {
        // Safe fallback for a missing/non-addressable RGB LED: normal LED state handling still works.
        static SingleLed led(RGB_LED_GPIO);
        return &led;
    }
};

DECLARE_BOARD(JRobotS3XiBody);
