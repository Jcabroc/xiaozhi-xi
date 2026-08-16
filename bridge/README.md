# Puente local de voz de Xi

Esta etapa conecta el protocolo WebSocket/Opus de XiaoZhi con un servidor local en el PC. La cadena elegida para el primer prototipo es:

1. micrófono I²S de Xi;
2. Silero VAD para detectar voz y pausas;
3. Faster Whisper `small`, fijado a español;
4. Qwen 3.5 4B mediante Ollama;
5. voz femenina `es-CL-CatalinaNeural` mediante Edge TTS;
6. audio Opus de vuelta al altavoz de Xi.

El modelo de lenguaje es local y no consume tokens de OpenAI. Edge TTS todavía necesita Internet, aunque no utiliza tokens de un LLM. Una etapa posterior puede sustituirlo por Piper u otra voz local.

## Estado validado

- Ollama 0.32.13 instalado.
- `qwen3.5:4b` descargado y ejecutándose en la RTX 3060.
- Respuesta caliente medida entre 0,9 y 1,5 segundos.
- Razonamiento desactivado con `reasoning_effort: none` en el endpoint compatible con OpenAI.
- Faster Whisper `small` descargado y cargado en CPU con cuantización INT8.
- Síntesis femenina chilena generada y decodificada correctamente.
- Handshake XiaoZhi WebSocket v1 validado.
- Servidor local: `ws://192.168.1.89:8000/xiaozhi/v1/`.
- OTA local: `http://192.168.1.89:8003/xiaozhi/ota/`.

## Selector de modos

Desde PowerShell:

```powershell
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1' -Mode Ollama
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1' -Mode XiaoZhi
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1' -Mode GPT
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiMode.ps1' -Mode Status
```

El selector administra el puente en segundo plano, registra su PID y reinicia Shi por `COM4` cuando está disponible. Se puede indicar otro puerto con `-Port COM7` o evitar el reinicio con `-NoDeviceReset`.

La primera carga de Qwen después de reiniciar el PC puede tardar entre 30 y 60 segundos. El selector precalienta el modelo antes de anunciar que el puente está disponible. `Start-XiBridge.ps1` se conserva como herramienta de diagnóstico en primer plano.

Antes de usar GPT se guarda la clave fuera del repositorio:

```powershell
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Set-XiOpenAIKey.ps1'
```

El modo GPT utiliza `gpt-5-mini` y consume tokens de OpenAI API. No representa ni comparte automáticamente el contexto de una tarea de Codex.

## Conmutación

La cara estable está protegida por la etiqueta Git `xi-stable-v1`. La integración se desarrolla en la rama `agent/xi-voice-bridge`.

El firmware instalado intenta primero la OTA local y usa su WebSocket cuando el PC responde. Si el puente no está disponible al arrancar, recurre a la OTA oficial de XiaoZhi. La conmutación sucede durante el inicio, no a mitad de una conversación.

La versión estable está respaldada fuera del árbol de compilación y el modo puente tiene su propio respaldo con SHA-256.
