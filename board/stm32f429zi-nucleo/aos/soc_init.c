/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#include <stdint.h>
#include "hal/hal.h"
#include "k_config.h"
#include "board.h"

#include "stm32f4xx_hal.h"

#include "hal_uart_stm32f4.h"
#include "hal_gpio_stm32f4.h"

#if defined (__CC_ARM) && defined(__MICROLIB)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
size_t g_iram1_start = 0x20000000;
size_t g_iram1_total_size = 0x00030000;
#elif defined(__ICCARM__)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#define GETCHAR_PROTOTYPE int fgetc(FILE *f)
#else
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#define GETCHAR_PROTOTYPE int __io_getchar(void)
#endif /* defined (__CC_ARM) && defined(__MICROLIB) */

uart_dev_t uart_0;

const gpio_mapping_t GPIO_MAPPINGConfig[GPIO_TABLE_SZ] =
{
    {GPIO_OUTPUT_PP, ON_BOARD_LED01, GPIOB, GPIO_PIN_0,  /*IRQ_NULL,*/GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_PinState_Reset},
    {GPIO_OUTPUT_PP, ON_BOARD_LED02, GPIOB, GPIO_PIN_7,  /*IRQ_NULL,*/GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_PinState_Reset},
    {GPIO_OUTPUT_PP, ON_BOARD_LED03, GPIOB, GPIO_PIN_14, /*IRQ_NULL,*/GPIO_PULLUP, GPIO_SPEED_FREQ_LOW, GPIO_PinState_Reset}
};

gpio_dev_t brd_gpio_table[] = 
{
    {ON_BOARD_LED01, OUTPUT_PUSH_PULL, NULL},
    {ON_BOARD_LED02, OUTPUT_PUSH_PULL, NULL},
    {ON_BOARD_LED03, OUTPUT_PUSH_PULL, NULL},
};


UART_MAPPING UART_MAPPING_TABLE[] =
{
    { PORT_UART_STD,     USART3, { USART3_IRQn,  0, 1,UART_OVERSAMPLING_16 } },
    { PORT_UART_AT,      USART6,  { USART6_IRQn , 0, 1,UART_OVERSAMPLING_16 } },
    { PORT_UART_RS485,   UART7, { UART7_IRQn, 0, 1,UART_OVERSAMPLING_16 } },
    { PORT_UART_SCANNER, UART4,  { UART4_IRQn,   0, 1,UART_OVERSAMPLING_16 } },
    { PORT_UART_LORA,    UART5,  { UART5_IRQn,   0, 1,UART_OVERSAMPLING_16 } },
};

void* i2c_mapping_table[] = {NULL, I2C1, I2C2, I2C3};

static void stduart_init(void);


static int32_t brd_gpio_init(void)
{
    int32_t i;
    int32_t ret = 0;
    
    for (i = 0; i < GPIO_TABLE_SZ; ++i) {
        ret = hal_gpio_init(&brd_gpio_table[i]);
        if (ret) {
            printf("gpio %d in gpio table init fail \r\n", i);
        }
    }

    return ret;
   
}

void stm32_soc_init(void)
{
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /**Configure the Systick interrupt time 
    */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/RHINO_CONFIG_TICKS_PER_SECOND);

    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, 0x0f, 0);
    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    /*default uart init*/
    stduart_init();
    /*gpio init*/
    brd_gpio_init();
    /*i2c pre init*/
    hal_i2c_pre_init();
#ifdef CONFIG_NET_LWIP
    /*ethernet if init*/
    lwip_tcpip_init();
#endif
}

static void stduart_init(void)
{
    uart_0.port = PORT_UART_STD;
    uart_0.config.baud_rate = STDIO_UART_BUADRATE;
    uart_0.config.data_width = DATA_WIDTH_8BIT;
    uart_0.config.flow_control = FLOW_CONTROL_DISABLED;
    uart_0.config.mode = MODE_TX_RX;
    uart_0.config.parity = NO_PARITY;
    uart_0.config.stop_bits = STOP_BITS_1;

    hal_uart_init(&uart_0);
}

/**
* @brief This function handles System tick timer.
*/
void SysTick_Handler(void)
{
  HAL_IncTick();
  krhino_intrpt_enter();
  krhino_tick_proc();
  krhino_intrpt_exit();
}

/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  if (ch == '\n') {
    //hal_uart_send(&console_uart, (void *)"\r", 1, 30000);
    hal_uart_send(&uart_0, (void *)"\r", 1, 30000);
  }
  hal_uart_send(&uart_0, &ch, 1, 30000);
  return ch;
}

/**
  * @brief  Retargets the C library scanf function to the USART.
  * @param  None
  * @retval None
  */
GETCHAR_PROTOTYPE
{
  /* Place your implementation of fgetc here */
  /* e.g. readwrite a character to the USART2 and Loop until the end of transmission */
  uint8_t ch = EOF;
  int32_t ret = -1;
  
  uint32_t recv_size;
  ret = hal_uart_recv_II(&uart_0, &ch, 1, &recv_size, HAL_WAIT_FOREVER);

  if (ret == 0) {
      return ch;
  } else {
      return -1;
  }
}

