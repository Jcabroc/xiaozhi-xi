#ifndef JROBOT_S3_XI_BODY_CONFIG_H
#define JROBOT_S3_XI_BODY_CONFIG_H

#include <driver/adc_types.h>
#include <driver/gpio.h>

// Audio: intentionally identical to the known-good jrobot-s3 prototype.
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_7
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_6
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_4
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_5

// External BOOT / push-to-talk. RESET is a physical switch wired to EN/RST.
#define BOOT_BUTTON_GPIO GPIO_NUM_0

// Shared I2C bus. PCA9685 is normally 0x40; MPU6050 is normally 0x68.
#define XI_BODY_I2C_SDA_GPIO GPIO_NUM_8
#define XI_BODY_I2C_SCL_GPIO GPIO_NUM_9
#define PCA9685_I2C_ADDRESS 0x40
#define IMU_I2C_ADDRESS 0x68

// TFT wiring only. The 128x160 controller is pending physical confirmation.
#define TFT_WIDTH 128
#define TFT_HEIGHT 160
#define TFT_SCK_GPIO GPIO_NUM_12
#define TFT_MOSI_GPIO GPIO_NUM_13
#define TFT_CS_GPIO GPIO_NUM_14
#define TFT_DC_GPIO GPIO_NUM_15
#define TFT_RST_GPIO GPIO_NUM_16
#define TFT_BL_GPIO GPIO_NUM_17
#define TFT_SPI_CLOCK_HZ (20 * 1000 * 1000)

// A 1S LiPo must be connected through a verified resistor divider; never direct.
#define BAT_ADC_GPIO GPIO_NUM_10
#define BAT_ADC_UNIT ADC_UNIT_1
#define BAT_ADC_CHANNEL ADC_CHANNEL_9

// ESP32-S3 has no native capacitive-touch peripheral. These are reserved GPIOs.
#define TOUCH_1_HEAD_GPIO GPIO_NUM_1
#define TOUCH_2_LEFT_GPIO GPIO_NUM_2
#define TOUCH_3_RIGHT_GPIO GPIO_NUM_3
#define TOUCH_4_BODY_GPIO GPIO_NUM_11

#define PIR_IN_GPIO GPIO_NUM_18
#define RGB_LED_GPIO GPIO_NUM_48

#endif  // JROBOT_S3_XI_BODY_CONFIG_H
