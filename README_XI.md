# Xi — XiaoZhi personalizada para jRobot S3

Xi es una versión personalizada de [XiaoZhi ESP32](https://github.com/78/xiaozhi-esp32) para una placa jRobot S3 con ESP32-S3, micrófono y altavoz I²S, pantalla OLED SSD1306 de 128 × 64 y LED RGB WS2812.

Esta rama conserva el nombre técnico XiaoZhi en el código, pero presenta al robot como **Xi**, una compañera robótica en español. La versión documentada aquí es el primer punto estable: conversación continua, captura de voz fiable, rostro expresivo y boca sincronizada exclusivamente con el audio reproducido por Xi.

## Estado estable

- Conversación en español con pausas naturales de hasta 3 segundos.
- Ganancia de entrada ajustada para el micrófono de la jRobot S3.
- Rostro OLED a pantalla completa, sin barra superior permanente.
- Ojos independientes con reposo, escucha, pensamiento, habla y parpadeo suave.
- Boca modulada mediante la energía PCM enviada realmente al altavoz.
- La voz del usuario anima los ojos de escucha, pero nunca la boca.
- Iconos grandes reservados para problemas que requieren atención.
- LED RGB usado como indicador de estado.

## Hardware y pinout

| Función | GPIO |
|---|---:|
| I²S BCLK | 4 |
| I²S WS/LRCK | 5 |
| Micrófono I²S DIN | 7 |
| Altavoz I²S DOUT | 6 |
| OLED SDA | 8 |
| OLED SCL | 9 |
| WS2812 integrado | 48 |
| Lámpara MCP de prueba | 18 |

El firmware también conserva definiciones para BOOT (GPIO 0), touch (GPIO 47) y volumen (GPIO 40/39), pero el prototipo estable no tiene botones físicos instalados.

## Estados del LED

| Estado | Indicación |
|---|---|
| Inicio | Azul intermitente |
| Configuración Wi‑Fi | Amarillo/naranja intermitente |
| Operativa y en reposo | Verde fijo |
| Conectando | Azul intermitente |
| Escuchando | Cian fijo |
| Hablando | Violeta fijo |
| Actualización | Naranja intermitente |
| Error fatal | Rojo intermitente |

## Compilación e instalación

El punto estable fue validado con ESP-IDF 5.5.4 para ESP32-S3.

```powershell
$env:IDF_PYTHON_ENV_PATH='C:\Espressif\python_env\idf5.5_py3.11_env'
. 'C:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1'
idf.py build
idf.py -p COM4 flash
```

El puerto `COM4` es el utilizado por el prototipo actual; debe sustituirse si Windows asigna otro.

## Personalidad

La personalidad conversacional se configura en la consola web de XiaoZhi y no contiene secretos dentro de este repositorio. Xi se identifica en femenino, llama al usuario Jota y prioriza respuestas breves, ingeniosas y naturales. El firmware no depende del nombre conversacional y continúa siendo compatible con XiaoZhi.

## Próximas etapas

1. Crear un puente de voz conmutado entre Xi, un modelo local mediante Ollama y Kira.
2. Estabilizar y medir latencia, reconocimiento, síntesis y cambio de proveedor.
3. Añadir movimiento solo después de estabilizar la voz.

## Documentación

- [Manual completo de instalación y uso](docs/MANUAL_SHI.md)
- [Arquitectura futura de conocimiento, robots y HUB](docs/ARQUITECTURA_HUB.md)
- [Puente local y herramientas de modo](bridge/README.md)

La propuesta mecánica contempla cinco servos: yaw y pitch de cabeza, dos brazos y una antena decorativa. Se recomienda un PCA9685 por I²C y una fuente independiente de 5–6 V para los servos, con tierra común con Xi. No deben alimentarse cinco servos desde el regulador o la entrada de la placa: sus picos de corriente pueden introducir ruido, reinicios o daños.

## Base y licencia

Este trabajo deriva del proyecto XiaoZhi ESP32 y conserva su historial y licencia. Consulta el README y la licencia originales para créditos, compatibilidad y condiciones completas.
