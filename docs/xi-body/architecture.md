# Xi Body v2 architecture and integration plan

The board layer owns pin assignments and bus setup. The existing core continues to own Wi-Fi, activation, audio service, and device state. The new variant currently keeps the original direct-I2S audio wiring and BOOT behavior.

## Safety boundary

- TFT: SPI bus only; no display driver is selected until the exact controller is confirmed.
- Servos: PCA9685 is reserved at I2C `0x40`; all six software limits are disabled. No startup motion and no PWM output occur.
- IMU: reserved at I2C `0x68`; no probe or polling occurs.
- Battery: GPIO10 is reserved only; divider values must be known before enabling a monitor.
- Touch: GPIO1/2/3/11 are reserves, not native ESP32-S3 touch inputs.

## Planned stages

1. Confirm TFT controller, orientation, offsets, and backlight polarity; enable the matching panel driver.
2. Choose and wire a compatible touch solution; integrate one input as a listening trigger.
3. Verify PCA9685 power, common ground, and one servo’s mechanical limits; explicitly enable only that channel.
4. Calibrate and enable the remaining five servos one at a time.
5. Add IMU, PIR, and battery sampling with debouncing/filtering.
6. Map `XiFaceState` values to the black-and-green face renderer and confirmed RGB hardware.
