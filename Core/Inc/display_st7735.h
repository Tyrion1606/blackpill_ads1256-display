#ifndef DISPLAY_ST7735_H
#define DISPLAY_ST7735_H

#include "main.h"
#include <stdint.h>
#include <stddef.h>

/* =========================
 * PINAGEM DO DISPLAY ST7735S
 * ========================= */
#define DISPLAY_ST7735_CHIP_SELECT_GPIO_PORT  GPIOB
#define DISPLAY_ST7735_CHIP_SELECT_GPIO_PIN   GPIO_PIN_4

#define DISPLAY_ST7735_DATA_COMMAND_GPIO_PORT GPIOB
#define DISPLAY_ST7735_DATA_COMMAND_GPIO_PIN  GPIO_PIN_6

#define DISPLAY_ST7735_RESET_GPIO_PORT        GPIOB
#define DISPLAY_ST7735_RESET_GPIO_PIN         GPIO_PIN_7

/* =========================
 * DIMENSOES DO DISPLAY
 * ========================= */
#define DISPLAY_ST7735_WIDTH_PIXELS  128U
#define DISPLAY_ST7735_HEIGHT_PIXELS 160U

/* =========================
 * COMANDOS DO DISPLAY ST7735
 * ========================= */
#define DISPLAY_ST7735_COMMAND_NO_OPERATION        0x00U
#define DISPLAY_ST7735_COMMAND_SOFTWARE_RESET      0x01U
#define DISPLAY_ST7735_COMMAND_SLEEP_OUT           0x11U
#define DISPLAY_ST7735_COMMAND_COLOR_MODE          0x3AU
#define DISPLAY_ST7735_COMMAND_MEMORY_ACCESS_CTRL  0x36U
#define DISPLAY_ST7735_COMMAND_COLUMN_ADDRESS_SET  0x2AU
#define DISPLAY_ST7735_COMMAND_ROW_ADDRESS_SET     0x2BU
#define DISPLAY_ST7735_COMMAND_MEMORY_WRITE        0x2CU
#define DISPLAY_ST7735_COMMAND_DISPLAY_ON          0x29U
#define DISPLAY_ST7735_COMMAND_INVERSION_ON        0x21U
#define DISPLAY_ST7735_COMMAND_INVERSION_OFF       0x20U

/* =========================
 * CORES RGB565
 * ========================= */
#define DISPLAY_COLOR_BLACK   0x0000U
#define DISPLAY_COLOR_WHITE   0xFFFFU
#define DISPLAY_COLOR_RED     0xF800U
#define DISPLAY_COLOR_GREEN   0x07E0U
#define DISPLAY_COLOR_BLUE    0x001FU
#define DISPLAY_COLOR_YELLOW  0xFFE0U
#define DISPLAY_COLOR_CYAN    0x07FFU
#define DISPLAY_COLOR_MAGENTA 0xF81FU

void DisplayST7735_SetPinsToSafeState(void);
void DisplayST7735_Initialize(void);
void DisplayST7735_FillScreen(uint16_t colorRgb565);
void DisplayST7735_DrawPixel(uint8_t xPosition, uint8_t yPosition, uint16_t colorRgb565);
void DisplayST7735_DrawFilledRectangle(uint8_t startXPosition,
                                       uint8_t startYPosition,
                                       uint8_t rectangleWidth,
                                       uint8_t rectangleHeight,
                                       uint16_t colorRgb565);
void DisplayST7735_DrawText(uint16_t startX, uint16_t startY, const char *text, uint16_t textColor);

#endif /* DISPLAY_ST7735_H */
