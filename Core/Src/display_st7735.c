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

static const uint8_t DisplayST7735_Font5x7[][5] =
{
    {0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x00,0x00,0x5F,0x00,0x00}, // 33 '!'
    {0x00,0x07,0x00,0x07,0x00}, // 34 '"'
    {0x14,0x7F,0x14,0x7F,0x14}, // 35 '#'
    {0x24,0x2A,0x7F,0x2A,0x12}, // 36 '$'
    {0x23,0x13,0x08,0x64,0x62}, // 37 '%'
    {0x36,0x49,0x55,0x22,0x50}, // 38 '&'
    {0x00,0x05,0x03,0x00,0x00}, // 39 '''
    {0x00,0x1C,0x22,0x41,0x00}, // 40 '('
    {0x00,0x41,0x22,0x1C,0x00}, // 41 ')'
    {0x14,0x08,0x3E,0x08,0x14}, // 42 '*'
    {0x08,0x08,0x3E,0x08,0x08}, // 43 '+'
    {0x00,0x50,0x30,0x00,0x00}, // 44 ','
    {0x08,0x08,0x08,0x08,0x00}, // 45 '-'
    {0x00,0x60,0x60,0x00,0x00}, // 46 '.'
    {0x20,0x10,0x08,0x04,0x02}, // 47 '/'
    {0x3E,0x51,0x49,0x45,0x3E}, // 48 '0'
    {0x00,0x42,0x7F,0x40,0x00}, // 49 '1'
    {0x42,0x61,0x51,0x49,0x46}, // 50 '2'
    {0x21,0x41,0x45,0x4B,0x31}, // 51 '3'
    {0x18,0x14,0x12,0x7F,0x10}, // 52 '4'
    {0x27,0x45,0x45,0x45,0x39}, // 53 '5'
    {0x3C,0x4A,0x49,0x49,0x30}, // 54 '6'
    {0x01,0x71,0x09,0x05,0x03}, // 55 '7'
    {0x36,0x49,0x49,0x49,0x36}, // 56 '8'
    {0x06,0x49,0x49,0x29,0x1E}, // 57 '9'
    {0x00,0x36,0x36,0x00,0x00}, // 58 ':'
    {0x00,0x56,0x36,0x00,0x00}, // 59 ';'
    {0x08,0x14,0x22,0x41,0x00}, // 60 '<'
    {0x14,0x14,0x14,0x14,0x14}, // 61 '='
    {0x00,0x41,0x22,0x14,0x08}, // 62 '>'
    {0x02,0x01,0x51,0x09,0x06}, // 63 '?'
    {0x32,0x49,0x79,0x41,0x3E}, // 64 '@'
    {0x7E,0x11,0x11,0x11,0x7E}, // 65 'A'
    {0x7F,0x49,0x49,0x49,0x36}, // 66 'B'
    {0x3E,0x41,0x41,0x41,0x22}, // 67 'C'
    {0x7F,0x41,0x41,0x22,0x1C}, // 68 'D'
    {0x7F,0x49,0x49,0x49,0x41}, // 69 'E'
    {0x7F,0x09,0x09,0x09,0x01}, // 70 'F'
    {0x3E,0x41,0x49,0x49,0x7A}, // 71 'G'
    {0x7F,0x08,0x08,0x08,0x7F}, // 72 'H'
    {0x00,0x41,0x7F,0x41,0x00}, // 73 'I'
    {0x20,0x40,0x41,0x3F,0x01}, // 74 'J'
    {0x7F,0x08,0x14,0x22,0x41}, // 75 'K'
    {0x7F,0x40,0x40,0x40,0x40}, // 76 'L'
    {0x7F,0x02,0x0C,0x02,0x7F}, // 77 'M'
    {0x7F,0x04,0x08,0x10,0x7F}, // 78 'N'
    {0x3E,0x41,0x41,0x41,0x3E}, // 79 'O'
    {0x7F,0x09,0x09,0x09,0x06}, // 80 'P'
    {0x3E,0x41,0x51,0x21,0x5E}, // 81 'Q'
    {0x7F,0x09,0x19,0x29,0x46}, // 82 'R'
    {0x46,0x49,0x49,0x49,0x31}, // 83 'S'
    {0x01,0x01,0x7F,0x01,0x01}, // 84 'T'
    {0x3F,0x40,0x40,0x40,0x3F}, // 85 'U'
    {0x1F,0x20,0x40,0x20,0x1F}, // 86 'V'
    {0x3F,0x40,0x38,0x40,0x3F}, // 87 'W'
    {0x63,0x14,0x08,0x14,0x63}, // 88 'X'
    {0x07,0x08,0x70,0x08,0x07}, // 89 'Y'
    {0x61,0x51,0x49,0x45,0x43}, // 90 'Z'
    {0x00,0x7F,0x41,0x41,0x00}, // 91 '['
    {0x02,0x04,0x08,0x10,0x20}, // 92 '\'
    {0x00,0x41,0x41,0x7F,0x00}, // 93 ']'
    {0x04,0x02,0x01,0x02,0x04}, // 94 '^'
    {0x40,0x40,0x40,0x40,0x40}, // 95 '_'
    {0x00,0x01,0x02,0x04,0x00}, // 96 '`'
    {0x20,0x54,0x54,0x54,0x78}, // 97 'a'
    {0x7F,0x48,0x44,0x44,0x38}, // 98 'b'
    {0x38,0x44,0x44,0x44,0x20}, // 99 'c'
    {0x38,0x44,0x44,0x48,0x7F}, // 100 'd'
    {0x38,0x54,0x54,0x54,0x18}, // 101 'e'
    {0x08,0x7E,0x09,0x01,0x02}, // 102 'f'
    {0x0C,0x52,0x52,0x52,0x3E}, // 103 'g'
    {0x7F,0x08,0x04,0x04,0x78}, // 104 'h'
    {0x00,0x44,0x7D,0x40,0x00}, // 105 'i'
    {0x20,0x40,0x44,0x3D,0x00}, // 106 'j'
    {0x7F,0x10,0x28,0x44,0x00}, // 107 'k'
    {0x00,0x41,0x7F,0x40,0x00}, // 108 'l'
    {0x7C,0x04,0x18,0x04,0x78}, // 109 'm'
    {0x7C,0x08,0x04,0x04,0x78}, // 110 'n'
    {0x38,0x44,0x44,0x44,0x38}, // 111 'o'
    {0x7C,0x14,0x14,0x14,0x08}, // 112 'p'
    {0x08,0x14,0x14,0x18,0x7C}, // 113 'q'
    {0x7C,0x08,0x04,0x04,0x08}, // 114 'r'
    {0x48,0x54,0x54,0x54,0x20}, // 115 's'
    {0x04,0x3F,0x44,0x40,0x20}, // 116 't'
    {0x3C,0x40,0x40,0x20,0x7C}, // 117 'u'
    {0x1C,0x20,0x40,0x20,0x1C}, // 118 'v'
    {0x3C,0x40,0x30,0x40,0x3C}, // 119 'w'
    {0x44,0x28,0x10,0x28,0x44}, // 120 'x'
    {0x0C,0x50,0x50,0x50,0x3C}, // 121 'y'
    {0x44,0x64,0x54,0x4C,0x44}, // 122 'z'
    {0x00,0x08,0x36,0x41,0x00}, // 123 '{'
    {0x00,0x00,0x7F,0x00,0x00}, // 124 '|'
    {0x00,0x41,0x36,0x08,0x00}, // 125 '}'
    {0x10,0x08,0x08,0x10,0x08}  // 126 '~'
};

static void DisplayST7735_DrawCharacter5x7(uint16_t startX, uint16_t startY, char character, uint16_t textColor)
{
    if (character < 32 || character > 126)
    {
        character = '?';
    }

    const uint8_t *glyphColumns = DisplayST7735_Font5x7[character - 32];

    for (uint8_t columnIndex = 0; columnIndex < 5; columnIndex++)
    {
        uint8_t columnBits = glyphColumns[columnIndex];

        for (uint8_t rowIndex = 0; rowIndex < 7; rowIndex++)
        {
            if ((columnBits & (1U << rowIndex)) != 0)
            {
                DisplayST7735_DrawPixel(
                    startX + columnIndex,
                    startY + rowIndex,
                    textColor
                );
            }
        }
    }
}

void DisplayST7735_DrawText(uint16_t startX, uint16_t startY, const char *text, uint16_t textColor)
{
    uint16_t cursorX = startX;
    uint16_t cursorY = startY;

    while (*text != '\0')
    {
        if (*text == '\n')
        {
            cursorX = startX;
            cursorY += 8;
            text++;
            continue;
        }

        if (*text == '\r')
        {
            text++;
            continue;
        }

        DisplayST7735_DrawCharacter5x7(cursorX, cursorY, *text, textColor);

        if (*text == ' ' || *text == '-')
        {
            cursorX += 5;
        }
        else
        {
            cursorX += 6;
        }

        text++;
    }
}
