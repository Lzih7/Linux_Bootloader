/**
  ******************************************************************************
  * @file    main.c
  * @brief   Application main program for STM32F401
  * @details This application demonstrates a simple LED blink at a slower rate
  *          than the bootloader to visually indicate that the application is running.
  ******************************************************************************
  */

#include "main.h"

/* Private variables */
static volatile uint32_t tick_counter = 0;

/**
  * @brief  Main program
  * @retval None
  */
void main(void)
{
  /* Relocate vector table to application region (important!) */
  /* Bootloader sets VTOR to 0x08000000, Application needs 0x08010000 */
  __disable_irq();
  SCB->VTOR = 0x08010000;  /* Application vector table offset */
  __enable_irq();
  
  /* Initialize system clocks and peripherals */
  system_init();
  
  /* Initialize GPIO for LED status indication */
  gpio_init();
  
  /* Initialize UART (optional, for debugging output) */
  // uart_init();
  
  /* Application main loop */
  while (1) {
    /* Toggle LED at slower rate than bootloader */
    led_toggle();
    delay_ms(LED_BLINK_DELAY_MS);
  }
}

/**
  * @brief  Initialize system clocks
  * @details Configures system clock to 84MHz using PLL from HSI
  * @retval None
  */
void system_init(void)
{
  /* Enable HSI */
  RCC->CR |= RCC_CR_HSION;
  while (!(RCC->CR & RCC_CR_HSIRDY));  /* Wait for HSI ready */
  
  /* Configure PLL for 84MHz system clock */
  /* PLL source = HSI (16MHz) */
  /* PLLM = 16, PLLN = 336, PLLP = 4 => 16MHz * 336 / 16 / 4 = 84MHz */
  RCC->PLLCFGR &= ~RCC_PLLCFGR_PLLSRC;  /* Clear PLL source */
  /* Note: Keeping default HSI for simplicity in this example */
  
  /* Update SystemCoreClock variable */
  SystemCoreClockUpdate();
  
  /* Vector table is already set to APP_START_ADDR by SystemInit() in startup code */
}

/**
  * @brief  Initialize GPIO for LED (PA5)
  * @retval None
  */
void gpio_init(void)
{
  /* Enable GPIOA clock */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  /* Configure PA5 as output */
  GPIOA->MODER &= ~(3 << (5 * 2));        /* Clear mode bits for PA5 */
  GPIOA->MODER |= (1 << (5 * 2));         /* Set PA5 to output mode */
  
  /* Configure PA5 output type (push-pull) */
  GPIOA->OTYPER &= ~(1 << 5);             /* Output push-pull */
  
  /* Configure PA5 speed (high speed) */
  GPIOA->OSPEEDR &= ~(3 << (5 * 2));      /* Clear speed bits */
  GPIOA->OSPEEDR |= (2 << (5 * 2));       /* High speed */
  
  /* Configure PA5 no pull-up/pull-down */
  GPIOA->PUPDR &= ~(3 << (5 * 2));        /* No pull-up/pull-down */
  
  /* Turn LED off initially */
  GPIOA->BSRR = (1 << (5 + 16));          /* Reset PA5 */
}

/**
  * @brief  Initialize UART2 for debugging (PA2: TX, PA3: RX)
  * @details Configures UART2 at 115200 baud, 8N1
  * @retval None
  */
void uart_init(void)
{
  /* Enable GPIOA clock for UART pins */
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  
  /* Enable USART2 clock */
  RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
  
  /* Configure PA2 (TX) and PA3 (RX) for UART */
  /* Set alternate function AF7 for USART2 */
  GPIOA->AFR[0] &= ~(0xF << (2 * 4));     /* Clear AF2 (PA2) */
  GPIOA->AFR[0] |= (7 << (2 * 4));        /* Set AF7 for PA2 */
  GPIOA->AFR[0] &= ~(0xF << (3 * 4));     /* Clear AF3 (PA3) */
  GPIOA->AFR[0] |= (7 << (3 * 4));        /* Set AF7 for PA3 */
  
  /* Set PA2 and PA3 to alternate function mode */
  GPIOA->MODER &= ~(3 << (2 * 2));        /* Clear PA2 mode */
  GPIOA->MODER |= (2 << (2 * 2));         /* Set PA2 to AF mode */
  GPIOA->MODER &= ~(3 << (3 * 2));        /* Clear PA3 mode */
  GPIOA->MODER |= (2 << (3 * 2));         /* Set PA3 to AF mode */
  
  /* Configure UART2 */
  /* Disable USART2 before configuration */
  USART2->CR1 &= ~USART_CR1_UE;
  
  /* Set baud rate to 115200 (assuming APB1 clock = 16MHz HSI/4 = 16MHz) */
  /* USART_BRR = f_clk / baud = 16MHz / 115200 ≈ 139 */
  USART2->BRR = 139;
  
  /* Configure 8N1: 8 data bits, no parity, 1 stop bit */
  /* Default: M=0 (8 data bits), PCE=0 (no parity), STOP=00 (1 stop bit) */
  
  /* Enable transmitter and receiver */
  USART2->CR1 |= USART_CR1_TE;
  USART2->CR1 |= USART_CR1_RE;
  
  /* Enable USART2 */
  USART2->CR1 |= USART_CR1_UE;
}

/**
  * @brief  Toggle LED state
  * @retval None
  */
void led_toggle(void)
{
  /* Toggle PA5 */
  if (GPIOA->ODR & (1 << 5)) {
    GPIOA->BSRR = (1 << (5 + 16));        /* Reset PA5 */
  } else {
    GPIOA->BSRR = (1 << 5);               /* Set PA5 */
  }
}

/**
  * @brief  Simple delay function
  * @param  ms: delay time in milliseconds
  * @retval None
  */
void delay_ms(uint32_t ms)
{
  /* Simple busy-wait delay */
  /* For 16MHz HSI: approximately 16000 cycles per millisecond */
  volatile uint32_t count;
  volatile uint32_t i;
  
  count = ms * 1600;
  
  for (i = 0; i < count; i++) {
    __NOP();
  }
}

/**
  * @brief  Send character via UART2
  * @param  ch: character to send
  * @retval None
  */
void uart_send_char(uint8_t ch)
{
  /* Wait until TXE flag is set */
  while (!(USART2->SR & USART_SR_TXE));
  
  /* Send character */
  USART2->DR = ch;
}

/**
  * @brief  Send string via UART2
  * @param  str: string to send
  * @retval None
  */
void uart_send_string(const char *str)
{
  while (*str) {
    uart_send_char(*str);
    str++;
  }
}
