#include "adc_ads1256.h"
#include "microsecond_delay.h"
#include "usb_cdc_serial.h"
#include "spi.h"

#include <stdio.h>

static void AdcADS1256_SelectAdcOnSpiBus(void);
static void AdcADS1256_ReleaseAdcFromSpiBus(void);
static void AdcADS1256_SendCommand(uint8_t commandByte);
static void AdcADS1256_WriteSingleRegister(uint8_t registerAddress, uint8_t registerValue);
static uint8_t AdcADS1256_WaitUntilDataIsReadyWithTimeout(uint32_t timeoutInMilliseconds);

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
  MicrosecondDelay_Wait(10);
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
  MicrosecondDelay_Wait(10);
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

  /* Seleciona entrada AIN0 medida contra AINCOM. */
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_MUX,
                                 ADC_ADS1256_CHANNEL_AIN0_AGAINST_AINCOM);

  /* ADCON = 0x00: clock out desligado, sensor detect desligado e ganho PGA = 1. */
  AdcADS1256_WriteSingleRegister(ADC_ADS1256_REGISTER_ADCON, 0x00U);

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

  if (muxRegisterValueReadBack != ADC_ADS1256_CHANNEL_AIN0_AGAINST_AINCOM)
  {
    UsbCdcSerial_WriteTextBlocking("ERRO: MUX do ADS1256 diferente do esperado\r\n");
    Error_Handler();
  }

  /*
   * Entra em modo de leitura continua.
   * Depois disso, a cada DRDY em nivel baixo, basta gerar clock SPI e ler 3 bytes.
   */
  AdcADS1256_SendCommand(ADC_ADS1256_COMMAND_READ_DATA_CONT);
  MicrosecondDelay_Wait(10);
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
