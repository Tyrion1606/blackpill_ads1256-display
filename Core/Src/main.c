/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "gpio.h"
#include "spi.h"
#include "usb_device.h"

#include "application_config.h"
#include "adc_ads1256.h"
#include "display_st7735.h"
#include "microsecond_delay.h"
#include "usb_cdc_serial.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Private type definitions --------------------------------------------------*/
typedef struct
{
  char text[USB_SERIAL_TRANSMIT_BUFFER_SIZE_BYTES];
  uint16_t usedLengthInBytes;
} UsbSerialTextBuffer;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

static void Application_SetVectorTableToStartAfterWeActBootloader(void);
static void Application_InitializeCubeGeneratedPeripherals(void);
static void Application_SetInitialPinStates(void);
static void Application_DrawDisplaySelfTest(void);
static void Application_RunContinuousAdcReadingLoop(void);
static void Application_AddTextToUsbBufferAndFlushIfNeeded(UsbSerialTextBuffer *usbSerialTextBuffer,
                                                           const char *textToAppend,
                                                           uint16_t textLengthInBytes);
static void Application_FlushUsbBufferIfNotEmpty(UsbSerialTextBuffer *usbSerialTextBuffer);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  Application_SetVectorTableToStartAfterWeActBootloader();

  HAL_Init();
  SystemClock_Config();

  Application_InitializeCubeGeneratedPeripherals();
  MicrosecondDelay_Initialize();
  Application_SetInitialPinStates();

  /*
   * Aguarda o computador reconhecer a USB CDC.
   * No Linux, a porta normalmente aparece como /dev/ttyACM0.
   */
  HAL_Delay(2000);

  DisplayST7735_Initialize();
  Application_DrawDisplaySelfTest();



  Application_RunContinuousAdcReadingLoop();
}

static void Application_SetVectorTableToStartAfterWeActBootloader(void)
{
  /*
   * Como a aplicacao esta em 0x08004000 por causa do bootloader HID da WeAct,
   * a tabela de vetores tambem precisa apontar para 0x08004000.
   */
  SCB->VTOR = APPLICATION_START_ADDRESS_IN_FLASH;
  __DSB();
  __ISB();
}

static void Application_InitializeCubeGeneratedPeripherals(void)
{
  MX_GPIO_Init();
  MX_SPI2_Init();
  MX_USB_DEVICE_Init();
  MX_SPI3_Init();
}

static void Application_SetInitialPinStates(void)
{
  AdcADS1256_SetPinsToSafeState();
  DisplayST7735_SetPinsToSafeState();

  HAL_GPIO_WritePin(APPLICATION_LED_GPIO_PORT,
                    APPLICATION_LED_GPIO_PIN,
                    GPIO_PIN_RESET);
}

static void Application_DrawDisplaySelfTest(void)
{
  DisplayST7735_FillScreen(DISPLAY_COLOR_BLACK);

  DisplayST7735_DrawFilledRectangle(5, 5, 10, 10, DISPLAY_COLOR_GREEN);
  DisplayST7735_DrawFilledRectangle(20, 5, 10, 10, DISPLAY_COLOR_BLUE);
  DisplayST7735_DrawFilledRectangle(35, 5, 10, 10, DISPLAY_COLOR_YELLOW);
}

static void Application_RunContinuousAdcReadingLoop(void)
{
  UsbCdcSerial_WriteTextBlocking("Blackpill STM32F411 + ADS1256\r\n");
  UsbCdcSerial_WriteTextBlocking("Formato CSV: sequencia,valor_bruto\r\n");
  UsbCdcSerial_WriteTextBlocking("Inicializando ADS1256...\r\n");
  AdcADS1256_Initialize();
  UsbCdcSerial_WriteTextBlocking("ADS1256 OK. Iniciando leitura.\r\n");

  UsbSerialTextBuffer usbSerialTextBuffer;
  memset(&usbSerialTextBuffer, 0, sizeof(usbSerialTextBuffer));

  uint32_t sampleSequenceNumber = 0;

  while (1)
  {
    AdcADS1256_WaitUntilDataIsReady();

    int32_t adcRawSignedValue = AdcADS1256_ReadRawSigned24BitValueContinuousMode();

    char csvLineText[64];
    int numberOfCharactersWrittenToCsvLine = snprintf(csvLineText,
                                                       sizeof(csvLineText),
                                                       "%lu,%ld\r\n",
                                                       (unsigned long)sampleSequenceNumber,
                                                       (long)adcRawSignedValue);

    if ((numberOfCharactersWrittenToCsvLine > 0) &&
        (numberOfCharactersWrittenToCsvLine < (int)sizeof(csvLineText)))
    {
      uint16_t csvLineLengthInBytes = (uint16_t)numberOfCharactersWrittenToCsvLine;

      Application_AddTextToUsbBufferAndFlushIfNeeded(&usbSerialTextBuffer,
                                                     csvLineText,
                                                     csvLineLengthInBytes);
    }

    sampleSequenceNumber++;

    if (usbSerialTextBuffer.usedLengthInBytes >
        (USB_SERIAL_TRANSMIT_BUFFER_SIZE_BYTES - USB_SERIAL_FLUSH_MARGIN_BYTES))
    {
      Application_FlushUsbBufferIfNotEmpty(&usbSerialTextBuffer);

      HAL_GPIO_TogglePin(APPLICATION_LED_GPIO_PORT,
                         APPLICATION_LED_GPIO_PIN);
    }
  }
}

static void Application_AddTextToUsbBufferAndFlushIfNeeded(UsbSerialTextBuffer *usbSerialTextBuffer,
                                                           const char *textToAppend,
                                                           uint16_t textLengthInBytes)
{
  if (textLengthInBytes >= USB_SERIAL_TRANSMIT_BUFFER_SIZE_BYTES)
  {
    Application_FlushUsbBufferIfNotEmpty(usbSerialTextBuffer);
    UsbCdcSerial_WriteBytesBlocking(textToAppend, textLengthInBytes);
    return;
  }

  uint16_t freeSpaceInUsbBuffer = USB_SERIAL_TRANSMIT_BUFFER_SIZE_BYTES -
                                  usbSerialTextBuffer->usedLengthInBytes;

  if (textLengthInBytes > freeSpaceInUsbBuffer)
  {
    Application_FlushUsbBufferIfNotEmpty(usbSerialTextBuffer);
  }

  char *destinationInsideUsbBuffer = usbSerialTextBuffer->text +
                                     usbSerialTextBuffer->usedLengthInBytes;

  memcpy(destinationInsideUsbBuffer, textToAppend, textLengthInBytes);

  usbSerialTextBuffer->usedLengthInBytes += textLengthInBytes;
}

static void Application_FlushUsbBufferIfNotEmpty(UsbSerialTextBuffer *usbSerialTextBuffer)
{
  if (usbSerialTextBuffer->usedLengthInBytes == 0U)
  {
    return;
  }

  UsbCdcSerial_WriteBytesBlocking(usbSerialTextBuffer->text,
                                  usbSerialTextBuffer->usedLengthInBytes);

  usbSerialTextBuffer->usedLengthInBytes = 0U;
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /*
   * Blackpill com cristal externo de 25 MHz.
   *
   * HSE  = 25 MHz
   * PLLM = 25  -> 1 MHz
   * PLLN = 192 -> 192 MHz
   * PLLP = 2   -> SYSCLK 96 MHz
   * PLLQ = 4   -> USB 48 MHz
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 |
                                RCC_CLOCKTYPE_PCLK2;

  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   /* 96 MHz */
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    /* 48 MHz */
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    /* 96 MHz */

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}
