#include "application_error.h"
#include "application_config.h"
#include "display_st7735.h"

void ApplicationError_BlinkLedForeverFast(void)
{
  while (1)
  {
    HAL_GPIO_WritePin(APPLICATION_LED_GPIO_PORT, APPLICATION_LED_GPIO_PIN, GPIO_PIN_RESET);
    HAL_Delay(80);

    HAL_GPIO_WritePin(APPLICATION_LED_GPIO_PORT, APPLICATION_LED_GPIO_PIN, GPIO_PIN_SET);
    HAL_Delay(80);
  }
}

void Error_Handler(void)
{
  //DisplayST7735_FillScreen(DISPLAY_COLOR_RED);
  __disable_irq();
  ApplicationError_BlinkLedForeverFast();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  (void)file;
  (void)line;
}
#endif /* USE_FULL_ASSERT */
