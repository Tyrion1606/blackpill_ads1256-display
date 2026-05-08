#include "usb_cdc_serial.h"
#include "usbd_cdc_if.h"

#include <string.h>

void UsbCdcSerial_WriteBytesBlocking(const char *textToSend, uint16_t numberOfBytesToSend)
{
  uint32_t transmissionStartTimeMilliseconds = HAL_GetTick();
  uint8_t *usbBytePointer = (uint8_t *)textToSend;
  uint8_t usbTransmitResult = USBD_BUSY;

  while (usbTransmitResult == USBD_BUSY)
  {
    usbTransmitResult = CDC_Transmit_FS(usbBytePointer, numberOfBytesToSend);

    if ((HAL_GetTick() - transmissionStartTimeMilliseconds) > 1000U)
    {
      return;
    }
  }
}

void UsbCdcSerial_WriteTextBlocking(const char *textToSend)
{
  uint16_t textLengthInBytes = (uint16_t)strlen(textToSend);
  UsbCdcSerial_WriteBytesBlocking(textToSend, textLengthInBytes);
}
