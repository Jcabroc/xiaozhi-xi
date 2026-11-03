# Audio local de interacciones físicas

El firmware actual no contiene clips de risa ni de reacción a sacudidas. La infraestructura de `PhysicalReactionAudio` queda lista, pero no reproduce sustitutos de prueba mientras falten los recursos.

## Añadir clips

Para el idioma configurado en `LANG_DIR`, añadir estos archivos OGG Opus mono:

- `main/assets/locales/<LANG_DIR>/xi_t2_laugh.ogg` — risa breve y frase juguetona.
- `main/assets/locales/<LANG_DIR>/xi_imu_shake.ogg` — reacción breve, amable y sin animar a sacudir a Lumi.

Conviene normalizar ambos clips a volumen moderado, con silencio inicial/final corto y una duración aproximada de 0,5–1,5 s. No sobrescribir los nombres de sonidos existentes. El generador deriva los símbolos `Lang::Sounds::OGG_XI_T2_LAUGH` y `OGG_XI_IMU_SHAKE` de esos nombres.

El demultiplexor lee `OpusHead` y extrae paquetes, pero `PlaySound` fija la duración de paquete a 60 ms y configura el decoder como mono. Por eso los clips deben ser **Ogg Opus, mono, paquetes de 60 ms**. Las muestras españolas incluidas se analizaron como Opus v1, mono, 24 kHz y 60 ms; los sonidos comunes usan 16 kHz, mono y 60 ms. Ambos valores de frecuencia ya están presentes en los recursos del firmware. El conversor incluido en `scripts/ogg_converter/xiaozhi_ogg_converter.py` genera `libopus`, 16 kbit/s, mono (`ac=1`), 16 kHz (`ar=16000`) y `frame_duration=60`.

Los OGG de idioma se pasan a `idf_component_register(... EMBED_FILES ...)`, por lo que quedan dentro de la **partición de aplicación** y aumentan `xiaozhi.bin`. No se agregan a la partición `assets`: esta contiene recursos gráficos y de fuente en la configuración normal de Xi Body. La compilación debe volver a configurar CMake (`idf.py reconfigure`) para que detecte archivos nuevos y regenere `main/assets/lang_config.h`.

## Arbitraje

Los sensores envían T2/IMU a una cola FreeRTOS de una entrada y no esperan a que se reproduzca audio. Una tarea de baja prioridad serializa las reacciones, descarta las que llegan durante una conversación o con colas de audio ocupadas, y aplica un enfriamiento global. El AudioService reserva atómicamente su canal local cuando todas las colas y las etapas de decodificación/salida están libres. Los paquetes locales llevan un identificador de generación: al cambiar XiaoZhi de estado o llegar audio del servidor, se cancelan y eliminan solo los paquetes locales pendientes. Como máximo puede terminar el pequeño bloque I2S que ya estaba escribiéndose.

La reproducción se desactiva con `CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO`. Si se agregaron los OGG, el sistema de recursos de idioma existente puede seguir incluyéndolos en la partición de aplicación aunque la reproducción esté desactivada. T1 sigue usando `StartListeningIfIdle()` y nunca solicita sonido.

En el PC de casa hay voces SAPI femeninas `Microsoft Sabina` (es-MX), `Microsoft Helena`/`Helena` y `Microsoft Laura` (es-ES), además de `Microsoft Sabina Desktop` (es-MX). Para estas reacciones se eligió `Microsoft Sabina` por su español mexicano. SAPI generó WAV PCM mono de 16 kHz; los WAV de prueba no tuvieron muestras recortadas. FFmpeg, ffprobe, opusenc y opusinfo ya estaban disponibles, así que no se instalaron dependencias. Los clips resultantes se validaron como Ogg Opus mono, frecuencia original de 16 kHz y paquetes de 60 ms. El build ESP-IDF vuelve a incluirlos en la app mediante `LANG_SOUNDS`/`EMBED_FILES` y, con `CONFIG_XI_BODY_PHYSICAL_REACTION_AUDIO`, activa sus símbolos para T2 e IMU.
