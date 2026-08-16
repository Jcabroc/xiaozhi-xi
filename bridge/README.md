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

## Arranque

Desde PowerShell:

```powershell
& 'C:\ESP32_Projects\xiaozhi-esp32\bridge\Start-XiBridge.ps1'
```

La primera carga de Qwen después de reiniciar el PC puede tardar entre 30 y 60 segundos. El lanzador precalienta el modelo antes de anunciar que el puente está disponible. Mantener la ventana abierta mantiene el servidor de Xi activo.

## Conmutación prevista

La cara estable está protegida por la etiqueta Git `xi-stable-v1`. La integración se desarrolla en la rama `agent/xi-voice-bridge`.

La siguiente modificación de firmware debe intentar primero la OTA local y usar su WebSocket cuando el PC responde. Si el puente no está disponible, Xi debe recurrir a la OTA oficial de XiaoZhi. Así, detener el lanzador devuelve a Xi al servicio de nube sin perder el firmware estable.

No se debe flashear esa conmutación hasta comprobar el retorno automático y guardar el binario estable actual.
