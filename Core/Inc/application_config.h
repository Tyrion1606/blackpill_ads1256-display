#ifndef APPLICATION_CONFIG_H
#define APPLICATION_CONFIG_H

#include "main.h"

/*
 * Endereco inicial da aplicacao na Flash.
 *
 * Este valor foi mantido em 0x08004000 porque voce esta usando o bootloader HID
 * da WeAct. O linker script tambem precisa estar configurado para iniciar nesse
 * mesmo endereco. Se o linker estiver em 0x08000000, a aplicacao pode sobrescrever
 * o bootloader ou iniciar de forma incorreta.
 */
#define APPLICATION_START_ADDRESS_IN_FLASH 0x08004000U

/*
 * LED onboard da Blackpill.
 * Na maioria das placas Blackpill, o LED do PC13 e ativo em nivel baixo:
 * GPIO_PIN_RESET = LED aceso
 * GPIO_PIN_SET   = LED apagado
 */
#define APPLICATION_LED_GPIO_PORT GPIOC
#define APPLICATION_LED_GPIO_PIN  GPIO_PIN_13

/*
 * Tamanho do buffer usado para acumular varias linhas CSV antes de enviar pela USB.
 * Enviar em blocos reduz o overhead em comparacao com enviar uma linha por vez.
 */
#define USB_SERIAL_TRANSMIT_BUFFER_SIZE_BYTES 256U //4096U

/*
 * Quando sobrar menos que esta margem no buffer, a aplicacao envia o conteudo
 * acumulado pela USB e zera o buffer.
 */
#define USB_SERIAL_FLUSH_MARGIN_BYTES 256U //64U

#endif /* APPLICATION_CONFIG_H */
