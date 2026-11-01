# Lista de integración física Xi Body v2

- [x] Registrar el montaje confirmado en [`pinout.md`](pinout.md).
- [x] Configurar GC9A01 240×240 (sin BL), I2C 8/9, INMP441 y MAX98357A, touch 1/2 y RGB 16/17/21.
- [x] Preparar el botón PTT en GPIO47; el botón aún falta instalar/soldar.
- [ ] Confirmar terminal común y polaridad del LED RGB externo antes de evaluar los colores.
- [ ] Encender primero con USB y revisar consola serie: pantalla, detección/respuesta MPU-6050 y eventos `TOCADO`/`LIBRE` de T1/T2.
- [ ] Confirmar terminal común y polaridad del LED RGB externo; mantener desactivada su prueba hasta entonces. Después verificar los tres colores y el WS2812 integrado.
- [ ] Tras instalar el botón GPIO47, probar PTT, grabación INMP441 y reproducción MAX98357A a volumen bajo.
- [ ] Probar el micrófono y el parlante por separado; ambos comparten BCLK GPIO4 y LRCLK/WS GPIO5.
- [ ] Si la pantalla enciende pero muestra colores incorrectos, verificar inversión RGB/BGR y orientación en el módulo.

T1 y T2 solo registran por consola los eventos de toque y liberación en ambos perfiles. No alternan la conversación. El perfil diagnóstico reproduce un aviso breve por el parlante al iniciar, muestra un aviso en pantalla y emite lecturas del MPU-6050. La secuencia RGB está desactivada por defecto y debe permanecer así hasta confirmar terminal común y polaridad. GPIO47 queda como PTT principal cuando se instale el botón; entonces se prueba la entrada del micrófono y la respuesta del parlante mediante el servicio de audio normal.

No flashear el perfil diagnóstico hasta elegirlo explícitamente. La compilación no valida soldaduras ni niveles eléctricos.
