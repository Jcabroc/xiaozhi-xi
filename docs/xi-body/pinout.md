# Xi Body v2 pinout

| Function | GPIO | Notes |
| --- | ---: | --- |
| I2S BCLK / mic SCK | 4 | Same mapping as `jrobot-s3` |
| I2S LRCK / mic WS | 5 | Same mapping as `jrobot-s3` |
| MAX98357A DIN | 6 | Speaker data |
| INMP441 SD | 7 | Microphone data |
| I2C SDA / SCL | 8 / 9 | PCA9685 `0x40`, IMU normally `0x68` |
| Battery ADC | 10 | ADC1 channel 9; required 1S-LiPo divider |
| Touch reserves | 1, 2, 3, 11 | ESP32-S3 has no native capacitive-touch peripheral; use an external controller or validate software sensing |
| TFT SCK / MOSI / CS | 12 / 13 / 14 | 128x160 SPI face |
| TFT DC / RST / BL | 15 / 16 / 17 | Controller pending confirmation |
| PIR input | 18 | Digital 3.3 V only; level-shift/divide a 5 V module output |
| BOOT / PTT | 0 | External button may parallel the board BOOT button |
| RESET | EN/RST | Physical reset only, not a firmware GPIO |
| RGB LED | 48 | Addressable-vs-single-color hardware still to verify |

Do not use GPIO19/20: they are reserved for USB. Do not power servos from the ESP32 3.3 V rail. Use a separate regulated 5 V supply and a common ground with the ESP32-S3 and PCA9685.

For the battery divider, choose resistor values so 4.2 V at the battery produces no more than 3.3 V at GPIO10; validate with a multimeter before enabling ADC reads.
