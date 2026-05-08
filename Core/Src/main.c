/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "spi.h"
#include "usb_device.h"
#include "gpio.h"
#include "usbd_cdc_if.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Private define ------------------------------------------------------------*/

#define APP_START_ADDRESS 0x08004000U

/* =========================
 * PINAGEM ADS1256
 * ========================= */
#define ADS_CS_PORT        GPIOB
#define ADS_CS_PIN         GPIO_PIN_12

#define ADS_DRDY_PORT      GPIOB
#define ADS_DRDY_PIN       GPIO_PIN_1

#define ADS_PDWN_PORT      GPIOB
#define ADS_PDWN_PIN       GPIO_PIN_0

/* LED onboard da Blackpill */
#define LED_PORT           GPIOC
#define LED_PIN            GPIO_PIN_13

/* Comandos ADS1256 */
#define ADS_CMD_WAKEUP     0x00
#define ADS_CMD_RDATA      0x01
#define ADS_CMD_RDATAC     0x03
#define ADS_CMD_SDATAC     0x0F
#define ADS_CMD_RREG       0x10
#define ADS_CMD_WREG       0x50
#define ADS_CMD_SELFCAL    0xF0
#define ADS_CMD_SYNC       0xFC
#define ADS_CMD_STANDBY    0xFD
#define ADS_CMD_RESET      0xFE

/* Registradores ADS1256 */
#define ADS_REG_STATUS     0x00
#define ADS_REG_MUX        0x01
#define ADS_REG_ADCON      0x02
#define ADS_REG_DRATE      0x03

/* Taxas de amostragem ADS1256 */
#define ADS_DRATE_30000SPS 0xF0
#define ADS_DRATE_15000SPS 0xE0
#define ADS_DRATE_7500SPS  0xD0
#define ADS_DRATE_2000SPS  0xB0
#define ADS_DRATE_1000SPS  0xA1
#define ADS_DRATE_100SPS   0x82
#define ADS_DRATE_10SPS    0x23
#define ADS_DRATE_2_5SPS   0x03 //2.5

#define ADS_DRATE_SELECTED  ADS_DRATE_1000SPS

/*
 * Canal inicial:
 * AIN0 contra AINCOM.
 */
#define ADS_MUX_AIN0_AINCOM 0x08

#define USB_TX_BUF_SIZE 2048 // 1024 - 2048 - 4096 - 8192


/* =========================
 * PINAGEM LCD ST7735S
 * ========================= */

#define LCD_CS_PORT    GPIOB
#define LCD_CS_PIN     GPIO_PIN_4

#define LCD_DC_PORT    GPIOB
#define LCD_DC_PIN     GPIO_PIN_6

#define LCD_RST_PORT   GPIOB
#define LCD_RST_PIN    GPIO_PIN_7

/* =========================
 * DIMENSÕES DO DISPLAY
 * ========================= */

#define ST7735_WIDTH   128
#define ST7735_HEIGHT  160

/* =========================
 * COMANDOS ST7735
 * ========================= */

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_DISPON  0x29
#define ST7735_INVON   0x21
#define ST7735_INVOFF  0x20

/* =========================
 * CORES RGB565
 * ========================= */

#define COLOR_BLACK    0x0000
#define COLOR_WHITE    0xFFFF
#define COLOR_RED      0xF800
#define COLOR_GREEN    0x07E0
#define COLOR_BLUE     0x001F
#define COLOR_YELLOW   0xFFE0
#define COLOR_CYAN     0x07FF
#define COLOR_MAGENTA  0xF81F

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

//funções para o ADS+serial
static void DWT_Init(void);
static void delay_us(uint32_t us);

static void usb_write_blocking(const char *data, uint16_t len);

static void ads_cs_low(void);
static void ads_cs_high(void);

static void ads_send_cmd(uint8_t cmd);
static void ads_write_reg(uint8_t reg, uint8_t value);
static uint8_t ads_read_reg(uint8_t reg);

static uint8_t ads_wait_drdy_timeout(uint32_t timeout_ms);
static void ads_wait_drdy(void);

static void ads_init(void);
static int32_t ads_read_raw24_continuous(void);

static void error_blink_fast(void);

//funções para o display
static void lcd_init(void);
static void lcd_reset(void);
static void lcd_write_command(uint8_t cmd);
static void lcd_write_data(uint8_t data);
static void lcd_write_data_buffer(uint8_t *data, uint16_t len);
static void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);
static void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color);
static void lcd_draw_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color);
static void lcd_fill_screen(uint16_t color);

static void lcd_cs_low(void);
static void lcd_cs_high(void);
static void lcd_dc_command(void);
static void lcd_dc_data(void);

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
	  /*
	   * Como seu app está em 0x08004000 por causa do bootloader WeAct,
	   * a tabela de vetores precisa apontar para 0x08004000.
	   */
	  SCB->VTOR = APP_START_ADDRESS;
	  __DSB();
	  __ISB();

	  HAL_Init();

	  SystemClock_Config();

	  MX_GPIO_Init();
	  MX_SPI2_Init();
	  MX_USB_DEVICE_Init();
	  MX_SPI3_Init();

	  DWT_Init();

	  /* Estado inicial seguro dos pinos do ADS1256. */
	  HAL_GPIO_WritePin(ADS_CS_PORT, ADS_CS_PIN, GPIO_PIN_SET);      // CS inativo
	  HAL_GPIO_WritePin(ADS_PDWN_PORT, ADS_PDWN_PIN, GPIO_PIN_SET);  // ADS acordado
	  HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);            // LED aceso

	  /* Estado inicial seguro dos pinos do Display. */
	  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);

  	  /*
	 * Aguarda o PC reconhecer a porta USB CDC.
	 * No Linux deve aparecer algo como /dev/ttyACM0.
	 */
	HAL_Delay(2000);

	lcd_init();
	lcd_fill_screen(COLOR_BLACK); // Tela preta para validar que o display respondeu.

	/* Alguns pixels/blocos extras para testar cores. */
	lcd_draw_block(5, 5, 10, 10, COLOR_GREEN);
	lcd_draw_block(20, 5, 10, 10, COLOR_BLUE);
	lcd_draw_block(35, 5, 10, 10, COLOR_YELLOW);

	usb_write_blocking("Blackpill STM32F411 + ADS1256\r\n", strlen("Blackpill STM32F411 + ADS1256\r\n"));
	usb_write_blocking("Formato CSV: seq,raw\r\n", strlen("Formato CSV: seq,raw\r\n"));
	usb_write_blocking("Inicializando ADS1256...\r\n", strlen("Inicializando ADS1256...\r\n"));
	ads_init();
	usb_write_blocking("ADS1256 OK. Iniciando leitura.\r\n", strlen("ADS1256 OK. Iniciando leitura.\r\n"));

	uint32_t seq = 0;
	char txbuf[USB_TX_BUF_SIZE];
	uint16_t txpos = 0;
	while (1)
	{
	    /*
	     * Aguarda nova amostra.
	     * DRDY é ativo em LOW.
	     */
	    ads_wait_drdy();

	    /*
	     * Lê amostra bruta signed de 24 bits.
	     */
	    int32_t raw = ads_read_raw24_continuous();

	    /*
	     * Formata como CSV:
	     * seq,raw
	     */
	    int n = snprintf(&txbuf[txpos],
	                     USB_TX_BUF_SIZE - txpos,
	                     "%lu,%ld\r\n",
	                     (unsigned long)seq,
	                     (long)raw);

	    if (n > 0)
	    {
	      txpos += (uint16_t)n;
	    }

	    seq++;

	    /*
	     * Envia em blocos, em vez de enviar linha por linha.
	     * Isso reduz overhead do USB CDC.
	     */
	    if (txpos > (USB_TX_BUF_SIZE - 64))
	    {
	      usb_write_blocking(txbuf, txpos);
	      txpos = 0;

	      /*
	       * LED C13 da Blackpill geralmente é ativo em LOW.
	       * Toggle indica que dados estão sendo enviados.
	       */
	      HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
	    }
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK |
      RCC_CLOCKTYPE_SYSCLK |
      RCC_CLOCKTYPE_PCLK1 |
      RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // 96 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;    // 48 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;    // 96 MHz

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* =========================
 * LCD - CONTROLE DE PINOS
 * ========================= */

static void lcd_cs_low(void)
{
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_RESET);
}

static void lcd_cs_high(void)
{
  HAL_GPIO_WritePin(LCD_CS_PORT, LCD_CS_PIN, GPIO_PIN_SET);
}

static void lcd_dc_command(void)
{
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_RESET);
}

static void lcd_dc_data(void)
{
  HAL_GPIO_WritePin(LCD_DC_PORT, LCD_DC_PIN, GPIO_PIN_SET);
}

/* =========================
 * LCD - ESCRITA SPI
 * ========================= */

static void lcd_write_command(uint8_t cmd)
{
  lcd_dc_command();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

static void lcd_write_data(uint8_t data)
{
  lcd_dc_data();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, &data, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

static void lcd_write_data_buffer(uint8_t *data, uint16_t len)
{
  lcd_dc_data();
  lcd_cs_low();

  if (HAL_SPI_Transmit(&hspi3, data, len, HAL_MAX_DELAY) != HAL_OK)
  {
    lcd_cs_high();
    Error_Handler();
  }

  lcd_cs_high();
}

/* =========================
 * LCD - INICIALIZAÇÃO
 * ========================= */

static void lcd_reset(void)
{
  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(10);

  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_RESET);
  HAL_Delay(50);

  HAL_GPIO_WritePin(LCD_RST_PORT, LCD_RST_PIN, GPIO_PIN_SET);
  HAL_Delay(120);
}

static void lcd_init(void)
{
  lcd_cs_high();

  lcd_reset();

  /*
   * Software reset.
   */
  lcd_write_command(ST7735_SWRESET);
  HAL_Delay(150);

  /*
   * Sai do modo sleep.
   */
  lcd_write_command(ST7735_SLPOUT);
  HAL_Delay(150);

  /*
   * Pixel format:
   * 0x05 = 16 bits por pixel, RGB565.
   */
  lcd_write_command(ST7735_COLMOD);
  lcd_write_data(0x05);
  HAL_Delay(10);

  /*
   * MADCTL controla orientação e ordem RGB/BGR.
   *
   * 0x00 = orientação básica.
   * Se a tela ficar invertida/girada ou cores trocadas,
   * ajustamos esse valor depois.
   */
  lcd_write_command(ST7735_MADCTL);
  lcd_write_data(0x00);

  /*
   * Liga o display.
   */
  lcd_write_command(ST7735_DISPON);
  HAL_Delay(150);
}

/* =========================
 * LCD - DESENHO
 * ========================= */

static void lcd_set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1)
{
  /*
   * Define intervalo de colunas.
   */
  lcd_write_command(ST7735_CASET);

  uint8_t col_data[4] = {
      0x00, x0,
      0x00, x1
  };

  lcd_write_data_buffer(col_data, 4);

  /*
   * Define intervalo de linhas.
   */
  lcd_write_command(ST7735_RASET);

  uint8_t row_data[4] = {
      0x00, y0,
      0x00, y1
  };

  lcd_write_data_buffer(row_data, 4);

  /*
   * Próximos bytes enviados serão dados de pixels.
   */
  lcd_write_command(ST7735_RAMWR);
}

static void lcd_draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
  if (x >= ST7735_WIDTH || y >= ST7735_HEIGHT)
  {
    return;
  }

  lcd_set_address_window(x, y, x, y);

  uint8_t color_data[2] = {
      (uint8_t)(color >> 8),
      (uint8_t)(color & 0xFF)
  };

  lcd_write_data_buffer(color_data, 2);
}

static void lcd_draw_block(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t color)
{
  for (uint8_t yy = y; yy < (uint8_t)(y + h); yy++)
  {
    for (uint8_t xx = x; xx < (uint8_t)(x + w); xx++)
    {
      lcd_draw_pixel(xx, yy, color);
    }
  }
}

static void lcd_fill_screen(uint16_t color)
{
  lcd_set_address_window(0, 0, ST7735_WIDTH - 1, ST7735_HEIGHT - 1);

  uint8_t color_data[2] = {
      (uint8_t)(color >> 8),
      (uint8_t)(color & 0xFF)
  };

  lcd_dc_data();
  lcd_cs_low();

  for (uint32_t i = 0; i < (uint32_t)ST7735_WIDTH * ST7735_HEIGHT; i++)
  {
    if (HAL_SPI_Transmit(&hspi3, color_data, 2, HAL_MAX_DELAY) != HAL_OK)
    {
      lcd_cs_high();
      Error_Handler();
    }
  }

  lcd_cs_high();
}

/* USER CODE BEGIN 4 PARA O ADS+SERIAL */

static void DWT_Init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CYCCNT = 0;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

static void delay_us(uint32_t us)
{
  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000U);

  while ((DWT->CYCCNT - start) < ticks)
  {
    __NOP();
  }
}

static void usb_write_blocking(const char *data, uint16_t len)
{
  uint32_t start = HAL_GetTick();

  while (CDC_Transmit_FS((uint8_t *)data, len) == USBD_BUSY)
  {
    if ((HAL_GetTick() - start) > 1000)
    {
      return;
    }
  }
}

static void ads_cs_low(void)
{
  HAL_GPIO_WritePin(ADS_CS_PORT, ADS_CS_PIN, GPIO_PIN_RESET);
}

static void ads_cs_high(void)
{
  HAL_GPIO_WritePin(ADS_CS_PORT, ADS_CS_PIN, GPIO_PIN_SET);
}

static void ads_send_cmd(uint8_t cmd)
{
  ads_cs_low();

  if (HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    ads_cs_high();
    Error_Handler();
  }

  ads_cs_high();

  delay_us(10);
}

static void ads_write_reg(uint8_t reg, uint8_t value)
{
  uint8_t tx[3];

  tx[0] = ADS_CMD_WREG | (reg & 0x0F);
  tx[1] = 0x00;
  tx[2] = value;

  ads_cs_low();

  if (HAL_SPI_Transmit(&hspi2, tx, 3, HAL_MAX_DELAY) != HAL_OK)
  {
    ads_cs_high();
    Error_Handler();
  }

  ads_cs_high();

  delay_us(10);
}

static uint8_t ads_read_reg(uint8_t reg)
{
  uint8_t tx[2];
  uint8_t value = 0;

  tx[0] = ADS_CMD_RREG | (reg & 0x0F);
  tx[1] = 0x00;

  ads_cs_low();

  if (HAL_SPI_Transmit(&hspi2, tx, 2, HAL_MAX_DELAY) != HAL_OK)
  {
    ads_cs_high();
    Error_Handler();
  }

  delay_us(10);

  if (HAL_SPI_Receive(&hspi2, &value, 1, HAL_MAX_DELAY) != HAL_OK)
  {
    ads_cs_high();
    Error_Handler();
  }

  ads_cs_high();

  delay_us(10);

  return value;
}

static uint8_t ads_wait_drdy_timeout(uint32_t timeout_ms)
{
  uint32_t start = HAL_GetTick();

  while (HAL_GPIO_ReadPin(ADS_DRDY_PORT, ADS_DRDY_PIN) == GPIO_PIN_SET)
  {
    if ((HAL_GetTick() - start) > timeout_ms)
    {
      return 0;
    }
  }

  return 1;
}

static void ads_wait_drdy(void)
{
  while (HAL_GPIO_ReadPin(ADS_DRDY_PORT, ADS_DRDY_PIN) == GPIO_PIN_SET)
  {
    __NOP();
  }
}

static void ads_init(void)
{
  ads_cs_high();

  /*
   * Mantém PDWN alto: ADS1256 ativo.
   */
  HAL_GPIO_WritePin(ADS_PDWN_PORT, ADS_PDWN_PIN, GPIO_PIN_SET);
  HAL_Delay(50);

  /*
   * Reset por comando SPI, pois seu módulo não expõe pino RESET.
   */
  ads_send_cmd(ADS_CMD_RESET);
  HAL_Delay(5);

  if (!ads_wait_drdy_timeout(1000))
  {
    usb_write_blocking("ERRO: timeout DRDY apos RESET\r\n",
                       strlen("ERRO: timeout DRDY apos RESET\r\n"));
    Error_Handler();
  }

  /*
   * Garante que não estamos em modo leitura contínua antes de configurar.
   */
  ads_send_cmd(ADS_CMD_SDATAC);
  HAL_Delay(2);

  /*
   * Seleciona canal AIN0 contra AINCOM.
   */
  ads_write_reg(ADS_REG_MUX, ADS_MUX_AIN0_AINCOM);

  /*
   * ADCON:
   * 0x00 = clock out off, sensor detect off, PGA gain = 1.
   */
  ads_write_reg(ADS_REG_ADCON, 0x00);

  /*
   * Configura taxa de amostragem.
   */
  ads_write_reg(ADS_REG_DRATE, ADS_DRATE_SELECTED);

  /*
   * Calibração interna.
   */
  ads_send_cmd(ADS_CMD_SELFCAL);

  if (!ads_wait_drdy_timeout(1000))
  {
    usb_write_blocking("ERRO: timeout DRDY apos SELFCAL\r\n",
                       strlen("ERRO: timeout DRDY apos SELFCAL\r\n"));
    Error_Handler();
  }

  /*
   * Verifica comunicação lendo de volta o registrador MUX.
   */
  uint8_t mux = ads_read_reg(ADS_REG_MUX);

  char msg[80];
  int n = snprintf(msg, sizeof(msg), "MUX lido: 0x%02X\r\n", mux);
  usb_write_blocking(msg, (uint16_t)n);

  if (mux != ADS_MUX_AIN0_AINCOM)
  {
    usb_write_blocking("ERRO: MUX diferente do esperado\r\n",
                       strlen("ERRO: MUX diferente do esperado\r\n"));
    Error_Handler();
  }

  /*
   * Entra em modo de leitura contínua.
   * Depois disso, a cada DRDY baixo, basta clockar 3 bytes pelo SPI.
   */
  ads_send_cmd(ADS_CMD_RDATAC);
  delay_us(10);
}

static int32_t ads_read_raw24_continuous(void)
{
  uint8_t tx[3] = {0xFF, 0xFF, 0xFF};
  uint8_t rx[3] = {0x00, 0x00, 0x00};

  ads_cs_low();

  if (HAL_SPI_TransmitReceive(&hspi2, tx, rx, 3, HAL_MAX_DELAY) != HAL_OK)
  {
    ads_cs_high();
    Error_Handler();
  }

  ads_cs_high();

  int32_t value = ((int32_t)rx[0] << 16) |
                  ((int32_t)rx[1] << 8) |
                  ((int32_t)rx[2]);

  /*
   * Extensão de sinal de 24 bits para 32 bits.
   */
  if (value & 0x800000)
  {
    value |= 0xFF000000;
  }

  return value;
}

static void error_blink_fast(void)
{
  while (1)
  {
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
    HAL_Delay(80);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
    HAL_Delay(80);
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  error_blink_fast();
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif
