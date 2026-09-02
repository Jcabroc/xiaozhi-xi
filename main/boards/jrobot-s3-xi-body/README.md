# jRobot S3 Xi Body v2

This isolated ESP32-S3 N16R8 variant is based on `jrobot-s3`. It preserves its direct I2S audio mapping and BOOT/push-to-talk behavior, while reserving Xi Body v2 hardware. `jrobot-s3` is not modified.

The 128x160 SPI display controller is **pending confirmation**. Therefore this initial firmware configures the SPI bus but intentionally uses `NoDisplay`; it does not send commands to an unknown panel. PCA9685 servo channels are declared with disabled safety limits and are never initialized or moved.

Build from the repository root:

```sh
python scripts/release.py jrobot-s3-xi-body --name jrobot-s3-xi-body
```

See [`docs/xi-body/`](../../../docs/xi-body/) for wiring, architecture, and staged integration.
