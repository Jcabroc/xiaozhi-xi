# Manual de instalación y uso de Shi

Este manual describe el prototipo físico **Shi**, cuyo nombre técnico en el firmware y en Git continúa siendo **Xi/XiaoZhi**. Está dirigido a una persona que nunca ha usado el dispositivo.

## 1. Qué es Shi

Shi es una asistente robótica basada en una jRobot S3 con ESP32-S3. La placa maneja el rostro, el LED, el micrófono, el altavoz, Wi-Fi y el protocolo de voz. El razonamiento puede venir de tres modos:

| Modo | Cerebro | Requiere PC | Internet | Coste de LLM |
|---|---|---:|---:|---:|
| `Ollama` | `qwen3.5:4b` local | Sí | Solo para Edge TTS | No usa tokens de OpenAI |
| `GPT` | OpenAI API | Sí | Sí | Sí, consume API |
| `XiaoZhi` | Nube oficial XiaoZhi | No después de configurarla | Sí | Según el servicio XiaoZhi |

La conmutación se realiza al iniciar o reiniciar Shi; no cambia de cerebro a mitad de una respuesta.

## 2. Hardware validado

- ESP32-S3, flash de 16 MB y PSRAM de 8 MB.
- OLED SSD1306 de 128 × 64.
- Micrófono digital I²S a 16 kHz.
- Altavoz I²S a 24 kHz.
- LED RGB WS2812 integrado.
- Wi-Fi de 2,4 GHz.
- Conversor USB CH343; en la unidad original aparece como `COM4`.

### Pinout actual

| Función | GPIO | Estado |
|---|---:|---|
| I²S BCLK compartido | 4 | En uso |
| I²S WS/LRCK compartido | 5 | En uso |
| Altavoz DOUT | 6 | En uso |
| Micrófono DIN | 7 | En uso |
| OLED SDA | 8 | En uso |
| OLED SCL | 9 | En uso |
| Lámpara MCP de prueba | 18 | Reservado por firmware |
| BOOT integrado | 0 | Reservado |
| Entrada touch declarada | 47 | Reservada por firmware |
| Volumen menos declarado | 39 | Reservado por firmware |
| Volumen más declarado | 40 | Reservado por firmware |
| LED WS2812 | 48 | En uso |

No hay botones externos instalados. El botón integrado BOOT sí puede alternar la conversación una vez iniciado el sistema.

### Pines propuestos para cinco servos

Esta asignación queda reservada para la siguiente etapa y debe confirmarse físicamente antes de conectar:

| Movimiento | Señal propuesta |
|---|---:|
| Cabeza yaw | GPIO 10 |
| Cabeza pitch | GPIO 11 |
| Brazo izquierdo | GPIO 12 |
| Brazo derecho | GPIO 13 |
| Antena | GPIO 14 |

Los GPIO solo transportan señal PWM. Para cinco servos se recomienda una fuente regulada separada de 5–6 V y 3–5 A, tierra común con la ESP32 y un condensador de 1000–2200 µF cerca de los servos. No se deben hacer pasar sus picos de corriente por el regulador ni por pistas pequeñas de la placa. Un PCA9685 por I²C es preferible si aparecen temblores, ruido de audio o falta de temporizadores.

## 3. Significado del LED

| Color y patrón | Estado |
|---|---|
| Azul rápido, 250 ms | Iniciando |
| Azul, 400 ms | Conectando al servicio |
| Azul lento, 500 ms | Activando sesión |
| Naranja, 500 ms | Configurando Wi-Fi |
| Naranja rápido, 250 ms | Actualizando firmware |
| Verde fijo | Operativa y en reposo |
| Cian fijo | Escuchando |
| Violeta fijo | Hablando |
| Rojo rápido, 200 ms | Error fatal |

Un cambio de color al pulsar BOOT confirma el evento del botón, pero no garantiza por sí solo que el servidor haya transcrito o contestado.

## 4. Instalación en un PC nuevo

### Requisitos

- Windows 10/11 de 64 bits.
- Git y GitHub CLI.
- Python 3.10.
- ESP-IDF 5.5.4 para compilar o flashear.
- Ollama.
- FFmpeg compartido y `libopus`.
- Red local donde PC y Shi puedan comunicarse.

Clonar los dos repositorios:

```powershell
git clone https://github.com/Jcabroc/xiaozhi-xi.git C:\ESP32_Projects\xiaozhi-esp32
git clone https://github.com/Jcabroc/xiaozhi-esp32-server.git `
  C:\Users\$env:USERNAME\Documents\ChatGPT\XiaoZhi\xiaozhi-esp32-server
```

Instalar Ollama y FFmpeg:

```powershell
winget install Ollama.Ollama
winget install BtbN.FFmpeg.LGPL.Shared.8.1
```

Crear el entorno del servidor e instalar dependencias:

```powershell
py -3.10 -m venv C:\ESP32_Projects\xi-bridge-venv
& C:\ESP32_Projects\xi-bridge-venv\Scripts\python.exe -m pip install -U pip
& C:\ESP32_Projects\xi-bridge-venv\Scripts\python.exe -m pip install -r `
  C:\Users\$env:USERNAME\Documents\ChatGPT\XiaoZhi\xiaozhi-esp32-server\main\xiaozhi-server\requirements.txt
```

Descargar el modelo local:

```powershell
ollama pull qwen3.5:4b
```

Copiar `bridge/xi-local.config.example.yaml` como `data/.config.yaml` dentro del servidor. Ajustar las rutas y la IP local del PC si son distintas. Los scripts actuales contienen rutas de la instalación original; en otro PC se deben editar las variables ubicadas al comienzo de cada `.ps1`.

## 5. Instalar el firmware

Conectar Shi con un cable USB de datos y localizar el puerto:

```powershell
[System.IO.Ports.SerialPort]::GetPortNames()
```

Compilar y flashear:

```powershell
$env:IDF_PYTHON_ENV_PATH='C:\Espressif\python_env\idf5.5_py3.11_env'
. 'C:\Espressif\frameworks\esp-idf-v5.5.4\export.ps1'
Set-Location C:\ESP32_Projects\xiaozhi-esp32
idf.py build
idf.py -p COM4 flash
```

Cambiar `COM4` si Windows asignó otro puerto. El respaldo estable `xi-stable-v1` debe conservarse antes de modificar la cara.

## 6. Elegir el modo

Abrir PowerShell y ejecutar uno de estos comandos:

```powershell
& C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1 -Mode Ollama
& C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1 -Mode XiaoZhi
& C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1 -Mode GPT
& C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1 -Mode Status
```

El selector intenta reiniciar la placa por `COM4`. Para otro puerto se usa `-Port COM7`. Con `-NoDeviceReset` pedirá reiniciarla manualmente.

### Preparar GPT

GPT queda desactivado hasta configurar una clave:

```powershell
& C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiOpenAIKey.ps1
```

La clave se solicita de forma oculta y se guarda como variable de usuario `OPENAI_API_KEY`; nunca debe añadirse a Git. El perfil utiliza `gpt-5-mini` mediante Chat Completions. Este modo consume tokens de la cuenta API y no equivale a esta tarea de Codex ni hereda automáticamente su conversación.

## 7. Conversar

1. Esperar el LED verde fijo.
2. Pulsar BOOT una vez o usar la palabra de activación configurada.
3. Hablar cuando el LED esté cian.
4. Dejar de hablar y esperar; Whisper suele tardar aproximadamente dos segundos y el modelo añade su propia latencia.
5. No volver a pulsar BOOT mientras procesa, porque eso puede abortar la respuesta.

En modo local su nombre hablado es **Shi**. `Xi` se conserva únicamente como identidad técnica del proyecto.

## 8. Archivos y copias importantes

| Elemento | Ubicación original |
|---|---|
| Firmware | `C:\ESP32_Projects\xiaozhi-esp32` |
| Servidor | `C:\Users\Gir\Documents\ChatGPT\XiaoZhi\xiaozhi-esp32-server` |
| Datos/modelos/logs | `C:\ESP32_Projects\xi-bridge-data` |
| Entorno Python | `C:\ESP32_Projects\xi-bridge-venv` |
| Respaldo estable | `C:\ESP32_Projects\XiBackups\xi-stable-v1-2026-08-15` |
| Respaldo puente | `C:\ESP32_Projects\XiBackups\xi-local-bridge-v1-2026-08-15` |

## 9. Solución de problemas

- **No aparece COM:** usar un cable USB de datos, revisar `USB-Enhanced-SERIAL CH343` y volver a conectar.
- **LED verde pero no responde:** comprobar `Set-XiMode.ps1 -Mode Status` y `http://127.0.0.1:8003/xiaozhi/ota/`.
- **Entiende palabras absurdas:** reducir ruido de impresoras, revisar la orientación del micrófono y consultar los WAV en `xi-bridge-data/tmp` cuando `delete_audio: false`.
- **Tarda mucho:** confirmar que Ollama fue precalentado y que el prompt no volvió a usar la plantilla china original.
- **Habla chino o pierde volumen:** revisar que la plantilla sea `bridge/xi-prompt-template.txt` y que Edge TTS use `es-CL-CatalinaNeural`.
- **Dice “once” como nombre:** el prompt hablado debe usar `Shi`; `Xi` queda solo para código.
- **Reinicios al añadir servos:** desconectar los servos y revisar fuente, tierra común, picos de corriente y ruido eléctrico.

## 10. Seguridad y mantenimiento

- No publicar `.config.yaml`, claves, voces privadas ni grabaciones.
- Mantener firmware, servidor y manual versionados por separado de modelos y logs.
- Probar primero cambios en una rama y conservar `xi-stable-v1`.
- Las órdenes futuras para mover robots deben exigir confirmación y límites físicos; las consultas de estado pueden ser de solo lectura.
