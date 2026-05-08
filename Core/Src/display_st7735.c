#include "display_st7735.h"
#include "spi.h"

static void DisplayST7735_SelectDisplayOnSpiBus(void);
static void DisplayST7735_ReleaseDisplayFromSpiBus(void);
static void DisplayST7735_SetCommandMode(void);
static void DisplayST7735_SetDataMode(void);
static void DisplayST7735_HardwareReset(void);
static void DisplayST7735_WriteCommandByte(uint8_t commandByte);
static void DisplayST7735_WriteDataByte(uint8_t dataByte);
static void DisplayST7735_WriteDataBytes(const uint8_t *dataBytesToSend, uint16_t numberOfBytesToSend);
static void DisplayST7735_SetAddressWindow(uint8_t startXPosition,
                                           uint8_t startYPosition,
                                           uint8_t endXPosition,
                                           uint8_t endYPosition);

void DisplayST7735_SetPinsToSafeState(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_CHIP_SELECT_GPIO_PORT,
                    DISPLAY_ST7735_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_SET);

  HAL_GPIO_WritePin(DISPLAY_ST7735_DATA_COMMAND_GPIO_PORT,
                    DISPLAY_ST7735_DATA_COMMAND_GPIO_PIN,
                    GPIO_PIN_SET);

  HAL_GPIO_WritePin(DISPLAY_ST7735_RESET_GPIO_PORT,
                    DISPLAY_ST7735_RESET_GPIO_PIN,
                    GPIO_PIN_SET);
}

static void DisplayST7735_SelectDisplayOnSpiBus(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_CHIP_SELECT_GPIO_PORT,
                    DISPLAY_ST7735_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_RESET);
}

static void DisplayST7735_ReleaseDisplayFromSpiBus(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_CHIP_SELECT_GPIO_PORT,
                    DISPLAY_ST7735_CHIP_SELECT_GPIO_PIN,
                    GPIO_PIN_SET);
}

static void DisplayST7735_SetCommandMode(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_DATA_COMMAND_GPIO_PORT,
                    DISPLAY_ST7735_DATA_COMMAND_GPIO_PIN,
                    GPIO_PIN_RESET);
}

static void DisplayST7735_SetDataMode(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_DATA_COMMAND_GPIO_PORT,
                    DISPLAY_ST7735_DATA_COMMAND_GPIO_PIN,
                    GPIO_PIN_SET);
}

static void DisplayST7735_WriteCommandByte(uint8_t commandByte)
{
  DisplayST7735_SetCommandMode();
  DisplayST7735_SelectDisplayOnSpiBus();

  if (HAL_SPI_Transmit(&hspi3, &commandByte, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    DisplayST7735_ReleaseDisplayFromSpiBus();
    Error_Handler();
  }

  DisplayST7735_ReleaseDisplayFromSpiBus();
}

static void DisplayST7735_WriteDataByte(uint8_t dataByte)
{
  DisplayST7735_SetDataMode();
  DisplayST7735_SelectDisplayOnSpiBus();

  if (HAL_SPI_Transmit(&hspi3, &dataByte, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    DisplayST7735_ReleaseDisplayFromSpiBus();
    Error_Handler();
  }

  DisplayST7735_ReleaseDisplayFromSpiBus();
}

static void DisplayST7735_WriteDataBytes(const uint8_t *dataBytesToSend, uint16_t numberOfBytesToSend)
{
  uint8_t *spiDataBytesToSend = (uint8_t *)dataBytesToSend;

  DisplayST7735_SetDataMode();
  DisplayST7735_SelectDisplayOnSpiBus();

  if (HAL_SPI_Transmit(&hspi3,
                       spiDataBytesToSend,
                       numberOfBytesToSend,
                       HAL_MAX_DELAY) != HAL_OK)
  {
    DisplayST7735_ReleaseDisplayFromSpiBus();
    Error_Handler();
  }

  DisplayST7735_ReleaseDisplayFromSpiBus();
}

static void DisplayST7735_HardwareReset(void)
{
  HAL_GPIO_WritePin(DISPLAY_ST7735_RESET_GPIO_PORT,
                    DISPLAY_ST7735_RESET_GPIO_PIN,
                    GPIO_PIN_SET);
  HAL_Delay(10);

  HAL_GPIO_WritePin(DISPLAY_ST7735_RESET_GPIO_PORT,
                    DISPLAY_ST7735_RESET_GPIO_PIN,
                    GPIO_PIN_RESET);
  HAL_Delay(50);

  HAL_GPIO_WritePin(DISPLAY_ST7735_RESET_GPIO_PORT,
                    DISPLAY_ST7735_RESET_GPIO_PIN,
                    GPIO_PIN_SET);
  HAL_Delay(120);
}

void DisplayST7735_Initialize(void)
{
  DisplayST7735_ReleaseDisplayFromSpiBus();
  DisplayST7735_HardwareReset();

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_SOFTWARE_RESET);
  HAL_Delay(150);

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_SLEEP_OUT);
  HAL_Delay(150);

  /* 0x05 = 16 bits por pixel, formato RGB565. */
  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_COLOR_MODE);
  DisplayST7735_WriteDataByte(0x05U);
  HAL_Delay(10);

  /*
   * MADCTL controla orientacao da tela e ordem RGB/BGR.
   * 0x00 foi mantido igual ao seu codigo original.
   * Se a imagem ficar girada, espelhada ou com cor trocada, este valor e o ponto
   * correto para ajustar depois.
   */
  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_MEMORY_ACCESS_CTRL);
  DisplayST7735_WriteDataByte(0x00U);

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_DISPLAY_ON);
  HAL_Delay(150);
}

static void DisplayST7735_SetAddressWindow(uint8_t startXPosition,
                                           uint8_t startYPosition,
                                           uint8_t endXPosition,
                                           uint8_t endYPosition)
{
  uint8_t columnAddressBytes[4] = {
      0x00U,
      startXPosition,
      0x00U,
      endXPosition
  };

  uint8_t rowAddressBytes[4] = {
      0x00U,
      startYPosition,
      0x00U,
      endYPosition
  };

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_COLUMN_ADDRESS_SET);
  DisplayST7735_WriteDataBytes(columnAddressBytes, sizeof(columnAddressBytes));

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_ROW_ADDRESS_SET);
  DisplayST7735_WriteDataBytes(rowAddressBytes, sizeof(rowAddressBytes));

  DisplayST7735_WriteCommandByte(DISPLAY_ST7735_COMMAND_MEMORY_WRITE);
}

void DisplayST7735_DrawPixel(uint8_t xPosition, uint8_t yPosition, uint16_t colorRgb565)
{
  if (xPosition >= DISPLAY_ST7735_WIDTH_PIXELS)
  {
    return;
  }

  if (yPosition >= DISPLAY_ST7735_HEIGHT_PIXELS)
  {
    return;
  }

  DisplayST7735_SetAddressWindow(xPosition, yPosition, xPosition, yPosition);

  uint8_t colorInBigEndianBytes[2] = {
      (uint8_t)(colorRgb565 >> 8),
      (uint8_t)(colorRgb565 & 0x00FFU)
  };

  DisplayST7735_WriteDataBytes(colorInBigEndianBytes, sizeof(colorInBigEndianBytes));
}

void DisplayST7735_DrawFilledRectangle(uint8_t startXPosition,
                                       uint8_t startYPosition,
                                       uint8_t rectangleWidth,
                                       uint8_t rectangleHeight,
                                       uint16_t colorRgb565)
{
  uint16_t endXPositionExclusive = (uint16_t)startXPosition + (uint16_t)rectangleWidth;
  uint16_t endYPositionExclusive = (uint16_t)startYPosition + (uint16_t)rectangleHeight;

  if (endXPositionExclusive > DISPLAY_ST7735_WIDTH_PIXELS)
  {
    endXPositionExclusive = DISPLAY_ST7735_WIDTH_PIXELS;
  }

  if (endYPositionExclusive > DISPLAY_ST7735_HEIGHT_PIXELS)
  {
    endYPositionExclusive = DISPLAY_ST7735_HEIGHT_PIXELS;
  }

  for (uint16_t currentYPosition = startYPosition;
       currentYPosition < endYPositionExclusive;
       currentYPosition++)
  {
    for (uint16_t currentXPosition = startXPosition;
         currentXPosition < endXPositionExclusive;
         currentXPosition++)
    {
      DisplayST7735_DrawPixel((uint8_t)currentXPosition,
                              (uint8_t)currentYPosition,
                              colorRgb565);
    }
  }
}

void DisplayST7735_FillScreen(uint16_t colorRgb565)
{
  uint8_t lastDisplayColumn = (uint8_t)(DISPLAY_ST7735_WIDTH_PIXELS - 1U);
  uint8_t lastDisplayRow = (uint8_t)(DISPLAY_ST7735_HEIGHT_PIXELS - 1U);

  uint8_t colorInBigEndianBytes[2] = {
      (uint8_t)(colorRgb565 >> 8),
      (uint8_t)(colorRgb565 & 0x00FFU)
  };

  uint32_t totalNumberOfPixels = (uint32_t)DISPLAY_ST7735_WIDTH_PIXELS *
                                 (uint32_t)DISPLAY_ST7735_HEIGHT_PIXELS;

  DisplayST7735_SetAddressWindow(0, 0, lastDisplayColumn, lastDisplayRow);

  DisplayST7735_SetDataMode();
  DisplayST7735_SelectDisplayOnSpiBus();

  for (uint32_t pixelIndex = 0; pixelIndex < totalNumberOfPixels; pixelIndex++)
  {
    if (HAL_SPI_Transmit(&hspi3,
                         colorInBigEndianBytes,
                         sizeof(colorInBigEndianBytes),
                         HAL_MAX_DELAY) != HAL_OK)
    {
      DisplayST7735_ReleaseDisplayFromSpiBus();
      Error_Handler();
    }
  }

  DisplayST7735_ReleaseDisplayFromSpiBus();
}
