#include <driver/gpio.h>
#include <driver/i2c_master.h>
#include <driver/spi_master.h>
#include <esp_lcd_gc9a01.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <memory>

#include "application.h"
#include "button.h"
#include "codecs/no_audio_codec.h"
#include "config.h"
#include "display/lcd_display.h"
#include "led/single_led.h"
#include "wifi_board.h"
#include "xi_body_peripherals.h"
#include "assets/lang_config.h"
#if CONFIG_XI_BODY_PHYSICAL_INTERACTIONS
#include "physical_interactions.h"
#endif
#if CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO
#include "physical_reaction_audio.h"
#endif

#define TAG "XiBodyV2"

class JRobotS3XiBody : public WifiBoard {
private:
    i2c_master_bus_handle_t i2c_bus_ = nullptr;
    Display* display_ = nullptr;
    Button boot_button_;
#if CONFIG_XI_BODY_PHYSICAL_INTERACTIONS
    std::unique_ptr<PhysicalInteractions> physical_interactions_;
#endif
#if CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO
    PhysicalReactionAudio physical_reaction_audio_;
#endif

    void InitializeGc9a01Display() {
        spi_bus_config_t bus = GC9A01_PANEL_BUS_SPI_CONFIG(TFT_SCK_GPIO, TFT_MOSI_GPIO,
                                                            TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t));
        ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
        esp_lcd_panel_io_handle_t io = nullptr;
        esp_lcd_panel_io_spi_config_t io_config =
            GC9A01_PANEL_IO_SPI_CONFIG(TFT_CS_GPIO, TFT_DC_GPIO, nullptr, nullptr);
        io_config.pclk_hz = TFT_SPI_CLOCK_HZ;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(SPI2_HOST, &io_config, &io));
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = TFT_RST_GPIO;
        panel_config.rgb_endian = LCD_RGB_ENDIAN_BGR;
        panel_config.bits_per_pixel = 16;
        esp_lcd_panel_handle_t panel = nullptr;
        ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, true));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));
        display_ = new SpiLcdDisplay(io, panel, TFT_WIDTH, TFT_HEIGHT, 0, 0, false, false, false);
    }

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
        InitializeGc9a01Display();
        ESP_LOGI(TAG, "GC9A01 initialized: 240x240, no backlight GPIO");
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
    }

    void InitializePhysicalInteractions() {
#if CONFIG_XI_BODY_PHYSICAL_INTERACTIONS
#if CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO
        if (!physical_reaction_audio_.Start()) {
            ESP_LOGE(TAG, "Failed to start physical reaction audio worker");
        }
#endif
        physical_interactions_ = std::make_unique<PhysicalInteractions>(i2c_bus_);
        if (!physical_interactions_->Start([this](PhysicalInteractionEvent event) {
                ESP_LOGI(TAG, "Physical event: %s", PhysicalInteractionEventName(event));
                if (event == PhysicalInteractionEvent::kTouch1Short) {
                    // A FreeRTOS event bit coalesces repeats and is checked by XiaoZhi's main task.
                    Application::GetInstance().StartListeningIfIdle();
                }
#if CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO
                else {
                    // Bounded queue; audio is produced only by the dedicated low-priority worker.
                    physical_reaction_audio_.Queue(event);
                }
#endif
            })) {
            ESP_LOGE(TAG, "Failed to start physical interaction task");
        }
#endif
    }

#if CONFIG_XI_BODY_DIAGNOSTIC
    static void DiagnosticTask(void* arg) {
        auto* self = static_cast<JRobotS3XiBody*>(arg);
        // Let Application::Initialize() finish setting up the display UI and audio service.
        vTaskDelay(pdMS_TO_TICKS(8000));
        Application::GetInstance().PlaySound(Lang::Sounds::OGG_SUCCESS);
#if CONFIG_XI_BODY_RGB_DIAGNOSTIC
        const gpio_num_t rgb[] = {RGB_RED_GPIO, RGB_GREEN_GPIO, RGB_BLUE_GPIO};
        gpio_config_t rgb_config = {.pin_bit_mask = (1ULL << RGB_RED_GPIO) | (1ULL << RGB_GREEN_GPIO) |
                                                    (1ULL << RGB_BLUE_GPIO),
                                    .mode = GPIO_MODE_OUTPUT,
                                    .pull_up_en = GPIO_PULLUP_DISABLE,
                                    .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                    .intr_type = GPIO_INTR_DISABLE};
        ESP_ERROR_CHECK(gpio_config(&rgb_config));
        for (auto pin : rgb) gpio_set_level(pin, RGB_LED_ACTIVE_LOW ? 1 : 0);
#endif
        self->display_->ShowNotification("DIAGNOSTICO XI BODY", 3000);

        uint8_t wake = 0;
        i2c_master_dev_handle_t imu = nullptr;
        i2c_device_config_t imu_config = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
                                          .device_address = IMU_I2C_ADDRESS,
                                          .scl_speed_hz = 100000};
        esp_err_t imu_result = i2c_master_bus_add_device(self->i2c_bus_, &imu_config, &imu);
        if (imu_result == ESP_OK) {
            uint8_t reg = 0x6B;
            imu_result = i2c_master_transmit_receive(imu, &reg, 1, &wake, 1, 100);
            if (imu_result == ESP_OK) {
                const uint8_t wake_command[] = {0x6B, 0x00};
                imu_result = i2c_master_transmit(imu, wake_command, sizeof(wake_command), 100);
            }
        }
        ESP_LOGI(TAG, "IMU probe address=0x68: %s, PWR_MGMT_1=0x%02X",
                 imu_result == ESP_OK ? "OK" : "FAILED", wake);

        while (true) {
#if CONFIG_XI_BODY_RGB_DIAGNOSTIC
            for (int i = 0; i < 3; ++i) {
                const int on_level = RGB_LED_ACTIVE_LOW ? 0 : 1;
                for (auto pin : rgb) gpio_set_level(pin, RGB_LED_ACTIVE_LOW ? 1 : 0);
                gpio_set_level(rgb[i], on_level);
                self->display_->ShowNotification(i == 0 ? "RGB: ROJO" : i == 1 ? "RGB: VERDE" : "RGB: AZUL", 700);
                vTaskDelay(pdMS_TO_TICKS(800));
                gpio_set_level(rgb[i], RGB_LED_ACTIVE_LOW ? 1 : 0);
            }
#endif
            if (imu_result == ESP_OK) {
                uint8_t reg = 0x3B;
                uint8_t data[14] = {};
                if (i2c_master_transmit_receive(imu, &reg, 1, data, sizeof(data), 100) == ESP_OK) {
                    ESP_LOGI(TAG, "MPU6050 accel raw X=%d Y=%d Z=%d",
                             static_cast<int16_t>((data[0] << 8) | data[1]),
                             static_cast<int16_t>((data[2] << 8) | data[3]),
                             static_cast<int16_t>((data[4] << 8) | data[5]));
                }
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
#endif

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
        InitializePhysicalInteractions();
        InitializeButtons();
#if CONFIG_XI_BODY_DIAGNOSTIC
        xTaskCreate(DiagnosticTask, "xi_body_diag", 4096, this, 2, nullptr);
#endif
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
        // Keep the on-board WS2812 as the application status indicator.
        static SingleLed led(WS2812_LED_GPIO);
        return &led;
    }
};

DECLARE_BOARD(JRobotS3XiBody);
