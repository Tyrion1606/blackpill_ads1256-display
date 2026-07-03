#include "adc_ads1256.h"
#include "microsecond_delay.h"
#include "usb_cdc_serial.h"
#include "spi.h"
#include "display_st7735.h"

#include <stdio.h>

#define ADC_ADS1256_COMMAND_SETTLE_DELAY_US        4U
#define ADC_ADS1256_REGISTER_WRITE_SETTLE_DELAY_US 2U
#define ADC_ADS1256_RDATA_RESPONSE_DELAY_US        7U

static void AdcADS1256_SelectAdcOnSpiBus(void);
static void AdcADS1256_ReleaseAdcFromSpiBus(void);
static void AdcADS1256_SendCommand(uint8_t commandByte);
static void AdcADS1256_WriteSingleRegister(uint8_t registerAddress, uint8_t registerValue);
static uint8_t AdcADS1256_WaitUntilDataIsReadyWithTimeout(uint32_t timeoutInMilliseconds);
static void AdcADS1256_DrawOperatingStatusOnDisplay(void);
static void AdcADS1256_FormatFixedPointCenti(char *destinationText,
                                             size_t destinationTextSize,
                                             uint32_t valueTimesOneHundred);
static int32_t AdcADS1256_ConvertThreeBytesToSigned24BitValue(const uint8_t adcResponseBytes[3]);
static int32_t AdcADS1256_ReadRawSigned24BitValueSingleMode(void);
static void AdcADS1256_StartConversionOnChannel(uint8_t channelIndex);

const AdcADS1256_ChannelConfig AdcADS1256_DifferentialChannels[ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT] = {
    {ADC_ADS1256_CHANNEL_AIN0_AGAINST_AIN1, "AIN0-AIN1"},
    {ADC_ADS1256_CHANNEL_AIN2_AGAINST_AIN3, "AIN2-AIN3"},
    {ADC_ADS1256_CHANNEL_AIN4_AGAINST_AIN5, "AIN4-AIN5"},
    {ADC_ADS1256_CHANNEL_AIN6_AGAINST_AIN7, "AIN6-AIN7"}
};

void AdcADS1256_SetPinsToSafeState(void)
{
  HAL_GPIO_WritePin(ADC_ADS1256_CHIP_SELECT_GPIO_PORT,
                    ADC_ADS1256_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_SET);

  HAL_GPIO_WritePin(ADC_ADS1256_POWER_DOWN_GPIO_PORT,
                    ADC_ADS1256_POWER_DOWN_GPIO_PIN,
                    GPIO_PIN_SET);
}

static void AdcADS1256_SelectAdcOnSpiBus(void)
{
  HAL_GPIO_WritePin(ADC_ADS1256_CHIP_SELECT_GPIO_PORT,
                    ADC_ADS1256_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_RESET);
}

static void AdcADS1256_ReleaseAdcFromSpiBus(void)
{
  HAL_GPIO_WritePin(ADC_ADS1256_CHIP_SELECT_GPIO_PORT,
                    ADC_ADS1256_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_SET);
}

static void AdcADS1256_SendCommand(uint8_t commandByte)
{
  AdcADS1256_SelectAdcOnSpiBus();

  if (HAL_SPI_Transmit(&hspi2, &commandByte, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  AdcADS1256_ReleaseAdcFromSpiBus();
  MicrosecondDelay_Wait(ADC_ADS1256_COMMAND_SETTLE_DELAY_US);
}

static void AdcADS1256_WriteSingleRegister(uint8_t registerAddress, uint8_t registerValue)
{
  uint8_t writeRegisterCommand = ADC_ADS1256_COMMAND_WRITE_REGISTER | (registerAddress & 0x0FU);
  uint8_t numberOfAdditionalRegistersToWrite = 0x00U;

  uint8_t spiMessageToWriteRegister[3] = {
      writeRegisterCommand,
      numberOfAdditionalRegistersToWrite,
      registerValue
  };

  AdcADS1256_SelectAdcOnSpiBus();

  if (HAL_SPI_Transmit(&hspi2,
                       spiMessageToWriteRegister,
                       sizeof(spiMessageToWriteRegister),
                       HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  AdcADS1256_ReleaseAdcFromSpiBus();
  MicrosecondDelay_Wait(ADC_ADS1256_REGISTER_WRITE_SETTLE_DELAY_US);
}

uint8_t AdcADS1256_ReadSingleRegister(uint8_t registerAddress)
{
  uint8_t readRegisterCommand = ADC_ADS1256_COMMAND_READ_REGISTER | (registerAddress & 0x0FU);
  uint8_t numberOfAdditionalRegistersToRead = 0x00U;
  uint8_t registerValueReadFromAdc = 0x00U;

  uint8_t spiMessageToRequestRegister[2] = {
      readRegisterCommand,
      numberOfAdditionalRegistersToRead
  };

  AdcADS1256_SelectAdcOnSpiBus();

  if (HAL_SPI_Transmit(&hspi2,
                       spiMessageToRequestRegister,
                       sizeof(spiMessageToRequestRegister),
                       HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  MicrosecondDelay_Wait(10);

  if (HAL_SPI_Receive(&hspi2, &registerValueReadFromAdc, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  AdcADS1256_ReleaseAdcFromSpiBus();
  MicrosecondDelay_Wait(10);

  return registerValueReadFromAdc;
}

static uint8_t AdcADS1256_WaitUntilDataIsReadyWithTimeout(uint32_t timeoutInMilliseconds)
{
  uint32_t waitingStartTimeMilliseconds = HAL_GetTick();

  while (HAL_GPIO_ReadPin(ADC_ADS1256_DATA_READY_GPIO_PORT,
                          ADC_ADS1256_DATA_READY_GPIO_PIN) == GPIO_PIN_SET)
  {
    if ((HAL_GetTick() - waitingStartTimeMilliseconds) > timeoutInMilliseconds)
    {
      return 0U;
    }
  }

  return 1U;
}

void AdcADS1256_WaitUntilDataIsReady(void)
{
  while (HAL_GPIO_ReadPin(ADC_ADS1256_DATA_READY_GPIO_PORT,
                          ADC_ADS1256_DATA_READY_GPIO_PIN) == GPIO_PIN_SET)
  {
    __NOP();
  }
}

void AdcADS1256_Initialize(void)
{
  AdcADS1256_ReleaseAdcFromSpiBus();

  /* Mantem o ADS1256 acordado. O pino PDWN precisa ficar em nivel alto. */
  HAL_GPIO_WritePin(ADC_ADS1256_POWER_DOWN_GPIO_PORT,
                    ADC_ADS1256_POWER_DOWN_GPIO_PIN,
                    GPIO_PIN_SET);

  HAL_Delay(50);

  /* Reset via comando SPI, porque o modulo usado nao expoe um pino RESET dedicado. */
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_RESET);
  HAL_Delay(5);

  if (AdcADS1256_WaitUntilDataIsReadyWithTimeout(1000U) == 0U)
  {
    UsbCdcSerial_WriteTextBlocking("ERRO: timeout no DRDY apos RESET do ADS1256\r\n");
    Error_Handler();
  }

  /* Sai de qualquer modo de leitura continua antes de configurar registradores. */
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_STOP_READ_CONT);
  HAL_Delay(2);

  /* Seleciona o canal configurado no .h. */
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_MUX,
                                 ADC_ADS1256_SELECTED_CHANNEL);

  /* ADCON = 0x00: clock out desligado, sensor detect desligado e ganho PGA = 1. */
  //AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_ADCON, 0x00U);
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_ADCON, ADC_ADS1256_SELECTED_PGA_GAIN);

  /* Define a taxa de amostragem configurada no .h. */
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_DRATE,
                                 ADC_ADS1256_SELECTED_DATA_RATE);

  /* Calibracao interna do ADS1256. */
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_SELF_CAL);

  if (AdcADS1256_WaitUntilDataIsReadyWithTimeout(1000U) == 0U)
  {
    UsbCdcSerial_WriteTextBlocking("ERRO: timeout no DRDY apos SELFCAL do ADS1256\r\n");
    Error_Handler();
  }

  uint8_t muxRegisterValueReadBack = AdcADS1256_ReadSingleRegister(ADC_ADS1256_REGISTER_MUX);

  char adcDiagnosticMessage[80];
  int numberOfCharactersWritten = snprintf(adcDiagnosticMessage,
                                           sizeof(adcDiagnosticMessage),
                                           "MUX do ADS1256 lido: 0x%02X\r\n",
                                           muxRegisterValueReadBack);

  if ((numberOfCharactersWritten > 0) &&
      (numberOfCharactersWritten < (int)sizeof(adcDiagnosticMessage)))
  {
    UsbCdcSerial_WriteBytesBlocking(adcDiagnosticMessage,
                                    (uint16_t)numberOfCharactersWritten);
  }

  if (muxRegisterValueReadBack != ADC_ADS1256_SELECTED_CHANNEL)
  {
    UsbCdcSerial_WriteTextBlocking("ERRO: MUX do ADS1256 diferente do esperado\r\n");
    DisplayST7735_DrawText(2, 20, "ERRO: MUX do ADS1256\ndiferente do esperado", DISPLAY_COLOR_RED);

    char displayMuxMessage[24];

    snprintf(displayMuxMessage,
             sizeof(displayMuxMessage),
             "MUX: 0x%02X",
             muxRegisterValueReadBack);

    DisplayST7735_DrawText(2, 40, displayMuxMessage, DISPLAY_COLOR_WHITE);
    Error_Handler();
  }

  AdcADS1256_DrawOperatingStatusOnDisplay();

  /*
   * Mantem o ADS1256 em modo RDATA para permitir troca do MUX entre leituras.
   * O modo RDATAC nao e usado na varredura dos quatro canais diferenciais.
   */
  MicrosecondDelay_Wait(10);

}

static void AdcADS1256_DrawOperatingStatusOnDisplay(void)
{
  char cyclingReadRateText[16];
  char cyclingFrameRateText[16];
  char cyclingReadRateAt1500KHzText[16];
  char cyclingFrameRateAt1500KHzText[16];
  char displayLineText[32];

  uint32_t cyclingFrameRateCenti =
      ADC_ADS1256_SELECTED_CYCLING_READS_PER_SECOND_CENTI /
      ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT;
  uint32_t cyclingFrameRateAt1500KHzCenti =
      ADC_ADS1256_SELECTED_CYCLING_READS_AT_1500KHZ_CENTI /
      ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT;

  AdcADS1256_FormatFixedPointCenti(cyclingReadRateText,
                                   sizeof(cyclingReadRateText),
                                   ADC_ADS1256_SELECTED_CYCLING_READS_PER_SECOND_CENTI);

  AdcADS1256_FormatFixedPointCenti(cyclingFrameRateText,
                                   sizeof(cyclingFrameRateText),
                                   cyclingFrameRateCenti);
  AdcADS1256_FormatFixedPointCenti(cyclingReadRateAt1500KHzText,
                                   sizeof(cyclingReadRateAt1500KHzText),
                                   ADC_ADS1256_SELECTED_CYCLING_READS_AT_1500KHZ_CENTI);

  AdcADS1256_FormatFixedPointCenti(cyclingFrameRateAt1500KHzText,
                                   sizeof(cyclingFrameRateAt1500KHzText),
                                   cyclingFrameRateAt1500KHzCenti);

  DisplayST7735_DrawText(2, 5, "ADS1256 Inicializado", DISPLAY_COLOR_WHITE);
  DisplayST7735_DrawText(2, 15, "Modo: 4 pares dif", DISPLAY_COLOR_CYAN);
  DisplayST7735_DrawText(2, 25, "Pares: 01 23 45 67", DISPLAY_COLOR_CYAN);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "DRATE: %lu SPS",
           (unsigned long)ADC_ADS1256_SELECTED_DATA_RATE_SPS);
  DisplayST7735_DrawText(2, 35, displayLineText, DISPLAY_COLOR_WHITE);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "PGA: x%u",
           (1U << ADC_ADS1256_SELECTED_PGA_GAIN));
  DisplayST7735_DrawText(2, 45, displayLineText, DISPLAY_COLOR_WHITE);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "MUX: %s leit/s",
           cyclingReadRateText);
  DisplayST7735_DrawText(2, 60, displayLineText, DISPLAY_COLOR_YELLOW);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "(1.5Mhz: %s)",
           cyclingReadRateAt1500KHzText);
  DisplayST7735_DrawText(2, 70, displayLineText, DISPLAY_COLOR_YELLOW);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "Frame: %s leit/s",
           cyclingFrameRateText);
  DisplayST7735_DrawText(2, 80, displayLineText, DISPLAY_COLOR_YELLOW);

  snprintf(displayLineText,
           sizeof(displayLineText),
           "(1.5Mhz: %s)",
           cyclingFrameRateAt1500KHzText);
  DisplayST7735_DrawText(2, 90, displayLineText, DISPLAY_COLOR_YELLOW);

  DisplayST7735_DrawText(2, 100, "Validado: ~1Ksps", DISPLAY_COLOR_YELLOW);

  DisplayST7735_DrawText(2, 115, "CSV format:", DISPLAY_COLOR_GREEN);
  DisplayST7735_DrawText(2, 125, "FRAME,s,t,d1,d2,d3,d4", DISPLAY_COLOR_GREEN);
}

static void AdcADS1256_FormatFixedPointCenti(char *destinationText,
                                             size_t destinationTextSize,
                                             uint32_t valueTimesOneHundred)
{
  snprintf(destinationText,
           destinationTextSize,
           "%lu.%02lu",
           (unsigned long)(valueTimesOneHundred / 100U),
           (unsigned long)(valueTimesOneHundred % 100U));
}

static int32_t AdcADS1256_ConvertThreeBytesToSigned24BitValue(const uint8_t adcResponseBytes[3])
{
  int32_t rawSigned24BitValue = ((int32_t)adcResponseBytes[0] << 16) |
                                ((int32_t)adcResponseBytes[1] << 8)  |
                                ((int32_t)adcResponseBytes[2]);

  /*
   * O ADS1256 retorna numero assinado em 24 bits.
   * Se o bit 23 estiver setado, o valor e negativo e precisa de extensao de sinal
   * para virar um int32_t correto.
   */
  if ((rawSigned24BitValue & 0x00800000L) != 0)
  {
    rawSigned24BitValue |= 0xFF000000L;
  }

  return rawSigned24BitValue;
}

static int32_t AdcADS1256_ReadRawSigned24BitValueSingleMode(void)
{
  uint8_t readDataCommand = ADC_ADS1256_COMMAND_READ_DATA;
  uint8_t dummyBytesSentOnlyToGenerateSpiClock[3] = {0xFFU, 0xFFU, 0xFFU};
  uint8_t adcResponseBytes[3] = {0x00U, 0x00U, 0x00U};

  AdcADS1256_SelectAdcOnSpiBus();

  if (HAL_SPI_Transmit(&hspi2, &readDataCommand, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  MicrosecondDelay_Wait(ADC_ADS1256_RDATA_RESPONSE_DELAY_US);

  if (HAL_SPI_TransmitReceive(&hspi2,
                              dummyBytesSentOnlyToGenerateSpiClock,
                              adcResponseBytes,
                              sizeof(adcResponseBytes),
                              HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  AdcADS1256_ReleaseAdcFromSpiBus();

  return AdcADS1256_ConvertThreeBytesToSigned24BitValue(adcResponseBytes);
}

int32_t AdcADS1256_ReadRawSigned24BitValueFromChannel(uint8_t muxRegisterValue)
{
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_MUX, muxRegisterValue);
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_SYNC);
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_WAKEUP);
  AdcADS1256_WaitUntilDataIsReady();

  return AdcADS1256_ReadRawSigned24BitValueSingleMode();
}

static void AdcADS1256_StartConversionOnChannel(uint8_t channelIndex)
{
  AdcADS1256_WriteSingleRegister(
      ADC_ADS1256_REGISTER_MUX,
      AdcADS1256_DifferentialChannels[channelIndex].muxRegisterValue);

  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_SYNC);
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_WAKEUP);
}

void AdcADS1256_ReadDifferentialChannelFrame(int32_t adcRawSignedValues[ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT])
{
  static uint8_t isPipelineInitialized = 0U;
  static uint8_t readyChannelIndex = 0U;

  if (isPipelineInitialized == 0U)
  {
    readyChannelIndex = 0U;
    AdcADS1256_StartConversionOnChannel(readyChannelIndex);
    AdcADS1256_WaitUntilDataIsReady();
    isPipelineInitialized = 1U;
  }

  for (uint8_t sampleIndex = 0U;
       sampleIndex < ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT;
       sampleIndex++)
  {
    uint8_t channelIndexToRead = readyChannelIndex;
    uint8_t nextChannelIndex = (uint8_t)(channelIndexToRead + 1U);

    if (nextChannelIndex >= ADC_ADS1256_DIFFERENTIAL_CHANNEL_COUNT)
    {
      nextChannelIndex = 0U;
    }

    AdcADS1256_WaitUntilDataIsReady();
    AdcADS1256_StartConversionOnChannel(nextChannelIndex);

    adcRawSignedValues[channelIndexToRead] =
        AdcADS1256_ReadRawSigned24BitValueSingleMode();

    readyChannelIndex = nextChannelIndex;
  }
}

int32_t AdcADS1256_ReadRawSigned24BitValueContinuousMode(void)
{
  uint8_t dummyBytesSentOnlyToGenerateSpiClock[3] = {0xFFU, 0xFFU, 0xFFU};
  uint8_t adcResponseBytes[3] = {0x00U, 0x00U, 0x00U};

  AdcADS1256_SelectAdcOnSpiBus();

  if (HAL_SPI_TransmitReceive(&hspi2,
                              dummyBytesSentOnlyToGenerateSpiClock,
                              adcResponseBytes,
                              sizeof(adcResponseBytes),
                              HAL_MAX_DELAY) != HAL_OK)
  {
    AdcADS1256_ReleaseAdcFromSpiBus();
    Error_Handler();
  }

  AdcADS1256_ReleaseAdcFromSpiBus();

  return AdcADS1256_ConvertThreeBytesToSigned24BitValue(adcResponseBytes);
}
