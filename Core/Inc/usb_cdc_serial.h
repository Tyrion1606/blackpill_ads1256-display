#ifndef USB_CDC_SERIAL_H
#define USB_CDC_SERIAL_H

#include "main.h"
#include <stdint.h>

void UsbCdcSerial_WriteBytesBlocking(const char *textToSend, uint16_t numberOfBytesToSend);
void UsbCdcSerial_WriteTextBlocking(const char *textToSend);

#endif /* USB_CDC_SERIAL_H */
