# XI-SISTERS-001 — Nacimiento de Lumi y Mira

**Fecha:** 2026-11-04
**Hito:** `XI-SISTERS-001`
**Repositorio:** `Jcabroc/xiaozhi-xi`
**Rama de release:** `feature/lumi-birthday-release-v1`
**Versión estable de Lumi:** `Lumi v1.0.0`
**Commit de firmware validado:** `7e65be63b12247f10b8b015629c72051956b4659`
**Tag de release:** `lumi-v1.0.0`, con destino exacto al commit de firmware indicado arriba (no al commit posterior de documentación).

Este hito registra el nacimiento de Lumi como primera versión estable de la familia Xi. Mira queda registrada como la segunda integrante proyectada. En esta fecha no tiene hardware ni firmware implementados.

## Identidad y configuración de Lumi

- **Placa:** ESP32-S3 Super Mini original; MAC `1C:29:04:BE:5E:68`.
- **Flash / PSRAM:** 4 MB de flash y 2 MB PSRAM en modo Quad.
- **SDK:** ESP-IDF 5.5.4.
- **Pantalla:** GC9A01 circular, 240×240, sin pin BL; orientación vertical correcta y sin espejo horizontal.
- **Sensores y audio:** MPU-6050 por I²C; INMP441 y MAX98357A en I²S compartido.
- **Indicadores:** RGB externo cátodo común, común a GND, activo en HIGH, GPIO16/17/21 con resistencias de 330 Ω por canal; WS2812 integrado en GPIO48.
- **Interacción:** touch capacitivo T1/T2 en GPIO1/GPIO2. El control de escucha por touch derecho y la reacción de cosquillas de T2 quedaron probados. El firmware incorpora reacción de IMU.
- **Particiones:** tabla custom `partitions/v2/4m.csv`; factory `0x10000` de tamaño `0x2F0000` (3.080.192 bytes), assets en `0x300000`.
- **Actualización:** instalación USB app-only en factory. No se usan actualizaciones OTA de aplicación en este hito.
| Módulo | Conexiones de la variante estable |
| --- | --- |
| GC9A01 redonda | CS 10, MOSI 11, SCK 12, DC 13, RST 14; sin BL |
| MPU-6050 | SDA 8, SCL 9 |
| INMP441 | SCK 4, WS 5, SD 6; 3,3 V; L/R a GND |
| MAX98357A | BCLK 4, LRCLK 5, DIN 7; 5 V |
| Touch T1/T2 | GPIO 1 / GPIO 2 |
| RGB externo | R 16, G 17, B 21; una resistencia de 330 Ω por canal; común GND |
| WS2812 integrado | GPIO 48 |
| PTT dedicado | GPIO 47; no se considera validado como pulsador independiente en este hito |

## Validación física declarada

El usuario confirmó personalmente las cinco comprobaciones finales:

| Prueba | Resultado confirmado |
| --- | --- |
| Cara | Perfecta; ojos bien orientados y sin textos innecesarios. |
| Reloj | Perfecto; centrado abajo. |
| RGB externo | Perfecto; indica correctamente los estados. |
| Conversación XiaoZhi | Perfecta. |
| Touch y cosquillas | Perfectos; probados físicamente. |

Además, el estado de trabajo recibido para este hito confirma IMU y reacciones de audio operativas. Estos resultados describen pruebas físicas reportadas por el usuario, no una certificación de todas las condiciones posibles.

## Capacidades verificadas y previstas

### Verificadas en Lumi v1.0.0

- Conversación de voz con XiaoZhi.
- Face Engine con ojos animados y orientación validada; reloj centrado en el borde inferior seguro.
- RGB externo como indicador de estados.
- Touch derecho para activar escucha y T2 para la reacción de cosquillas.
- MPU-6050 y reacciones físicas con audio local.
- Instalación de la aplicación por USB en la partición factory.

### Pendientes o futuras

- Touch izquierdo y su comportamiento adicional quedan para una versión futura.
- El pulsador dedicado en GPIO47 no se considera validado por las cinco pruebas de este hito; no confundirlo con el touch que activa escucha.
- Nuevas expresiones y animaciones faciales.
- Mejoras de robustez y ajuste del touch.
- Mejoras de IMU y reconocimiento de gestos.
- Evolución de voz y conversación.
- Posible boca animada.
- Mira: integrante proyectada, sin hardware o firmware implementados.
- Reutilización en Lumi y Mira de módulos Xi que maduren en futuras iteraciones.

No hay cambio aprobado de firmware para estas mejoras dentro de `XI-SISTERS-001`.

## Firmware estable y rollback

La aplicación estable es `xiaozhi.bin`, compilada para el commit `7e65be63b12247f10b8b015629c72051956b4659`.

- **SHA-256 estable:** `C694E484707AC4F35331295BD3AC3BB2EE526E81E5FFB59E18BBB067390808D8`
- **Tamaño:** 2.776.832 bytes.
- **Offset:** `0x10000`; tamaño factory `0x2F0000`.
- **Rollback inmediatamente anterior:** commit `4c750746b6fb6d5da12fe17f14f7c2445632d5bf`.
- **SHA-256 rollback:** `95D37D4CCA5B7BAD3D6E7ACA3BEE4F8BFEEE70D5024E732C566B4DC68B33010F`.

El paquete de recuperación contiene ambos binarios, manifiesto, estas instrucciones y una copia de este documento. No contiene NVS, credenciales Wi-Fi ni datos personales del niño. La configuración privada permanece en la placa y no debe copiarse a repositorios o paquetes.

## Recuperación por USB (solo aplicación)

1. Conectar Lumi y detectar el puerto serial actual. Consultar la MAC directamente con esptool; continuar únicamente si es `1C:29:04:BE:5E:68`.
2. Confirmar flash de 4 MB y que la tabla instalada tenga factory en `0x10000`, tamaño `0x2F0000`.
3. Validar el SHA-256 del binario estable o del rollback elegido.
4. Cerrar el monitor serial y escribir exclusivamente la aplicación. Ejemplo con esptool 4.x:

   ```powershell
   python -m esptool --chip esp32s3 -p COMx -b 115200 write_flash --flash_mode dio --flash_size 4MB --flash_freq 80m --verify 0x10000 xiaozhi.bin
   ```

5. Confirmar `Hash of data verified` y dejar que esptool reinicie la placa.

No ejecutar `erase_flash`/`--erase-all`; no escribir bootloader, tabla, NVS, OTA ni assets. Si la identidad o verificación falla, detenerse sin reintentos automáticos. El rollback se instala con el mismo procedimiento sustituyendo el archivo.

## Evolución de la familia Xi

`XI-SISTERS-001` fija a Lumi v1.0.0 como primera versión estable. Los siguientes pasos podrán tomar módulos maduros de Xi, endurecer Touch y gestos, evolucionar las expresiones, IMU, voz y conversación, y evaluar una boca animada. Mira permanece como diseño proyectado hasta que se definan y validen su hardware y firmware.
