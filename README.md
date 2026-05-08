# Blackpill STM32F411 - Projeto modular legivel

Este pacote separa o codigo em modulos mais claros, com nomes mais longos e descritivos.
A intencao aqui e facilitar leitura humana, nao encurtar o codigo.

## Estrutura

Core/Inc/application_config.h
- Configuracoes gerais da aplicacao: endereco inicial da Flash, LED onboard e tamanho do buffer USB.

Core/Inc/application_error.h + Core/Src/application_error.c
- Tratamento de erro e pisca rapido do LED.
- Contem o Error_Handler(). Nao deixe outro Error_Handler() duplicado no main.c.

Core/Inc/adc_ads1256.h + Core/Src/adc_ads1256.c
- Driver simples do ADC ADS1256 usando SPI2.
- Prefixo usado: AdcADS1256_...

Core/Inc/display_st7735.h + Core/Src/display_st7735.c
- Driver simples do display TFT ST7735 usando SPI3.
- Prefixo usado: DisplayST7735_...

Core/Inc/microsecond_delay.h + Core/Src/microsecond_delay.c
- Delay em microssegundos usando DWT->CYCCNT.

Core/Inc/usb_cdc_serial.h + Core/Src/usb_cdc_serial.c
- Escrita bloqueante pela USB CDC.

Core/Src/main.c
- Fica apenas com a orquestracao da aplicacao:
  inicializa perifericos, inicializa display, inicializa ADS1256 e entra no loop de leitura CSV.

## Como aplicar no STM32CubeIDE

1. Copie os arquivos .h para Core/Inc.
2. Copie os arquivos .c para Core/Src.
3. Substitua o seu Core/Src/main.c pelo main.c deste pacote.
4. Confirme que nao ficou outro Error_Handler() duplicado em outro arquivo gerado manualmente.
5. Faca Clean Project e depois Build Project.

## Observacoes importantes

- O ADS1256 esta usando hspi2.
- O display ST7735 esta usando hspi3.
- A aplicacao esta configurada para iniciar em 0x08004000 por causa do bootloader HID da WeAct.
- O linker script tambem precisa estar configurado para iniciar em 0x08004000.
