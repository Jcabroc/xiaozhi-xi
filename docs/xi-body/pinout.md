# Xi Body v2: montaje confirmado

Este pinout corresponde únicamente al perfil `jrobot-s3-xi-body`. No cambiar los perfiles hermanos.

| Módulo | GPIO | Conexión / notas |
| --- | --- | --- |
| GC9A01 redonda | CS 10, MOSI 11, SCK 12, DC 13, RST 14 | 240×240; no tiene pin BL |
| MPU-6050 | SDA 8, SCL 9 | I2C, dirección esperada `0x68` |
| INMP441 | SCK 4, WS 5, SD 6 | 3,3 V; L/R a GND |
| MAX98357A | BCLK 4, LRCLK 5, DIN 7 | Alimentación 5 V; comparte reloj I2S con el micrófono |
| Touch T1/T2 | 1 / 2 | Touch capacitivo nativo del ESP32-S3 |
| LED RGB externo | R 16, G 17, B 21 | Una resistencia de 330 Ω en cada canal |
| Botón para hablar | 47 y GND | Configurado como entrada PTT; falta instalar/soldar el botón |
| WS2812 integrado | 48 | LED de estado de la placa; sin cable externo |

GPIO10 ya no se reserva para batería porque es CS de la pantalla. El perfil no habilita monitor de batería. El PIR (GPIO18) y PCA9685 siguen sin inicializarse en este perfil.

El mapa no especifica el terminal común ni la polaridad eléctrica del LED RGB. La prueba de los canales externos está desactivada por defecto (`XI_BODY_RGB_DIAGNOSTIC=n`); confirmados terminal común y polaridad, puede habilitarse en la configuración de compilación diagnóstica. `RGB_LED_ACTIVE_LOW` permite seleccionar la polaridad correcta. El WS2812 de GPIO48 sigue siendo el LED de estado normal y no forma parte de esos tres canales.

No usar GPIO19/20, reservados para USB. No alimentar el MAX98357A desde 3,3 V.
