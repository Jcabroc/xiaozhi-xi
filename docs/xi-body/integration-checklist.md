# Xi Body v2 integration checklist

- [ ] Confirm TFT controller (ST7735, ST7789, ILI9163, GC9107, or other), SPI mode, offsets, and reset/backlight polarity.
- [ ] Confirm PCA9685 address and servo 5 V power supply; connect common ground.
- [ ] Measure servo pulse limits mechanically before changing any `enabled` flag.
- [ ] Confirm IMU address and interrupt wiring, if used.
- [ ] Select an external capacitive-touch controller or validate an alternative for GPIO1/2/3/11.
- [ ] Verify PIR output is 3.3 V-safe.
- [ ] Verify battery divider voltage at 4.2 V before enabling ADC.
- [ ] Confirm GPIO48 LED electrical type before enabling RGB colour control.
- [ ] Run the old `jrobot-s3` build and the new `jrobot-s3-xi-body` build independently.
