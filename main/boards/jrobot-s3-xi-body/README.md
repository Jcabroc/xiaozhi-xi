# jRobot S3 Xi Body v2

Perfil aislado ESP32-S3 para Xi Body. La placa conectada durante la prueba reportó 4 MB de flash y 2 MB de PSRAM; ambos builds usan la tabla de particiones de 4 MB y configuran PSRAM Quad. Su pinout confirmado está en [`docs/xi-body/pinout.md`](../../../docs/xi-body/pinout.md). El perfil normal inicializa la pantalla GC9A01 240×240 y mantiene el WS2812 integrado como LED de estado.

El botón PTT GPIO47 está configurado, pero aún falta instalar/soldar. El RGB externo usa GPIO16/17/21; confirma la polaridad de su terminal común antes de probar colores. El perfil hermano `jrobot-s3` no se modifica.

Compilaciones desde la raíz:

```powershell
python scripts/release.py jrobot-s3-xi-body --name jrobot-s3-xi-body
python scripts/release.py jrobot-s3-xi-body --name jrobot-s3-xi-body-diagnostic
```

El perfil diagnóstico muestra un aviso en pantalla, reproduce un sonido y sondea el MPU-6050. T1/T2 solo reportan toque/liberación por consola en ambos perfiles; no controlan la conversación. La prueba RGB está desactivada hasta confirmar la polaridad del LED externo. Para probar micrófono y parlante usa el servicio de audio después de instalar el botón PTT. Ver [`docs/xi-body/integration-checklist.md`](../../../docs/xi-body/integration-checklist.md).
