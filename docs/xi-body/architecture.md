# Xi Body v2: arquitectura de integración

La asignación de pines vive únicamente en `main/boards/jrobot-s3-xi-body/config.h`; el perfil `jrobot-s3` y el repositorio `xiaozhi-esp32-server` no forman parte de este hardware.

## Perfil normal

- GC9A01 240×240 mediante el controlador `esp_lcd_gc9a01`, SPI2 y sin señal de BL.
- Audio I2S full-duplex compartido: BCLK/SCK 4, LRCK/WS 5, micrófono SD 6 y parlante DIN 7.
- I2C SDA8/SCL9 disponible para MPU-6050; dirección esperada `0x68`.
- GPIO1/2 reservados para touch capacitivo nativo ESP32-S3.
- WS2812 integrado en GPIO48 conserva el indicador de estado. RGB externo está en GPIO16/17/21.
- PTT preparado en GPIO47; falta montar el botón entre GPIO47 y GND.

## Perfil diagnóstico

T1/T2 solo registran por consola los cambios de toque/liberación en los perfiles normal y diagnóstico; no cambian el estado de conversación. `jrobot-s3-xi-body-diagnostic` muestra un aviso en la pantalla, reproduce un aviso corto por el parlante y lee el acelerómetro MPU-6050. La prueba RGB externa está desactivada; solo se puede habilitar con `XI_BODY_RGB_DIAGNOSTIC` después de confirmar terminal común y polaridad. GPIO47 permanece preparado como PTT físico y todavía falta soldar su botón.

## Límites conocidos

- No reservar GPIO10 para batería: ahora es CS de GC9A01.
- No inicializar PIR GPIO18 ni PCA9685; sus etapas permanecen fuera de esta variante.
- La polaridad/terminal común del RGB externo no aparece en el mapa de montaje. El código inicia en activo alto; `RGB_LED_ACTIVE_LOW` permite cambiar a activo bajo si el hardware es ánodo común.
- La auditoría del modelo semántico de expresiones para OLED, GC9A01 y TFT verde se abordará después; esta integración usa la pantalla estándar y no rediseña ese modelo compartido.
