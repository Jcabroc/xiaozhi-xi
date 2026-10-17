# Auditoría de expresiones, pantallas y cableado pendiente

**Fecha:** 2026-10-17
**Repositorio:** `xiaozhi-xi`
**Rama:** `feature/xi-body-v2`
**Commit revisado:** `31e2d00f44e52c43f03f4a34fc98bf068512bb38`

Esta auditoría es de solo lectura. No confirma el montaje físico ni propone un pinout. La tabla de conexiones al final queda vacía hasta cotejar fotos claras de la SuperMini y los módulos.

## Resumen

- La cara expresiva actual de Xi está implementada como geometría procedural en el renderer OLED de 128×64. No encontré un paquete de imágenes de la cara de Xi que pueda reutilizarse en otras resoluciones.
- El flujo común de la aplicación usa cadenas para `SetEmotion` y `SetStatus`; el enum `XiFaceState` de Xi Body no tiene referencias fuera de su declaración. Todavía no hay un modelo semántico común conectado a los renderers.
- Xi Body devuelve `NoDisplay`. No hay un renderer Xi habilitado para la TFT 128×160 ni una integración de placa identificada para GC9A01 240×240.
- El renderer LCD genérico presenta imágenes de emojis cargadas desde assets y puede recurrir a iconos. Esas colecciones no son el rostro de Xi.
- No se cambió firmware, configuración GPIO ni el repositorio hermano `xiaozhi-esp32-server`.

## Flujo actual de expresión

La clase base `Display` define métodos separados para estado, emoción, actividad de voz y nivel de audio. La aplicación envía el texto de emoción recibido del servidor directamente al renderer. En paralelo, los cambios de estado llaman `SetStatus`.

El OLED amistoso de 128×64 dibuja ojos y boca en coordenadas fijas. Diferencia visualmente reposo, escucha, pensamiento, conexión, sueño y habla. Durante la escucha, `SetVoiceActivity` anima los ojos. Durante el habla, el nivel de boca proviene de la energía PCM enviada al altavoz.

La lista de expresiones que el OLED acepta es más amplia que las expresiones que dibuja de forma distinta. Nombres como `happy`, `sad`, `angry`, `laughing` y `confused` pasan la validación, pero caen en la geometría de reposo. Los nombres desconocidos se intentan mostrar como iconos; si no hay icono disponible, aparece el icono de advertencia genérico.

`XiFaceState` incluye conceptos adicionales (`kHappy`, `kLowBattery`, `kTouched`, `kMoved` y otros), pero actualmente no forma parte del flujo de la aplicación ni de la interfaz de `Display`.

## Renderers y recursos por pantalla

| Pantalla | Estado del código | Recursos / brecha observada |
|---|---|---|
| OLED monocroma 128×64 | `OledDisplay` activa la cara procedural amistosa en la variante jrobot-s3. | Es la única cara Xi integrada. Las formas están limitadas por coordenadas de 128×64; no hay una descripción de rostro independiente de la resolución. |
| TFT redonda GC9A01 240×240 | No encontré selección de placa o controlador GC9A01 para esta variante. Hay un renderer LCD LVGL genérico. | Ese renderer consume imágenes emoji por nombre; no adapta automáticamente la cara procedural del OLED ni aporta una composición circular Xi. |
| TFT 128×160 | Xi Body reserva el bus SPI, pero usa `NoDisplay` mientras el controlador no esté confirmado. | No se inicializa un panel ni se renderizan expresiones en esta variante. |

Las colecciones Twemoji de 32 y 64 píxeles están declaradas en el código común; el renderer LCD también carga colecciones de imágenes desde el paquete de assets. No encontré archivos gráficos de rostro Xi dentro de `main/`. Esos emojis pueden servir como contenido gráfico genérico, pero no conservan por sí solos la identidad visual de Xi.

## Brechas para continuar

1. Definir el vocabulario semántico compartido a partir de las expresiones que Xi ya usa, sin rediseñar sus caritas.
2. Conectar ese vocabulario a una interfaz de renderer, conservando la animación OLED y permitiendo implementaciones para TFT redonda y TFT 128×160.
3. Enumerar qué estados tienen una representación visual real en cada renderer y qué estados necesitan recursos o equivalentes.
4. Confirmar el controlador físico de cada TFT antes de implementar su driver. Esta auditoría no determina cableado ni pines.

## Pendiente: tabla de cableado físico

Completar cada fila mirando el montaje y la serigrafía de la placa. No rellenar desde el pinout del firmware.

| Módulo | Pin del módulo | GPIO físico | Alimentación | GND |
|---|---|---|---|---|
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |
|  |  |  |  |  |

## Para retomar desde casa

El archivo queda guardado en `docs/xi-body/`. La rama revisada es `feature/xi-body-v2`; el commit de referencia es `31e2d00f44e52c43f03f4a34fc98bf068512bb38`. El siguiente insumo físico pendiente son fotos claras de la SuperMini y los módulos. Después de cotejar la tabla de conexiones, se podrá especificar el siguiente cambio sin deducir el montaje a partir del firmware.
