/**
  ******************************************************************************
  * @file    main.c
  * @brief   Bootloader main program for STM32F401
  * @details This bootloader initializes the system and jumps to the application
  *          located at 0x08010000 if valid. Otherwise, it stays in bootloader mode.
  ******************************************************************************
  */

#include "main.h"

/* Private function prototypes */
static int32_t validate_application(void);
static void bootloader_jump_to_app(void);
static void bootloader_error_loop(void);

/* Private variables */
static volatile uint32_t boot_timeout = 0;
static volatile uint8_t boot_flag = 0;

/**
  * @brief  Main program
  * @retval None
  */
int main(void)
{
  /* Initialize system clocks and peripherals */
  system_init();
  
  /* Initialize GPIO for LED status indication */
  gpio_init();
  
  /* Check if application is valid */
  if (validate_application() == BOOTLOADER_OK) {
    /* Application is valid, jump to it after short delay */
    boot_timeout = BOOT_TIMEOUT_MS;
    
    /* Wait for timeout (LED fast blink) */
    while (boot_timeout > 0) {
      led_toggle();
      delay_ms(100);
      boot_timeout -= 100;
    }
    
    /* Jump to application */
    bootloader_jump_to_app();
  } else {
    /* No valid application, stay in bootloader (LED solid on) */
    bootloader_error_loop();
  }
  
  /* Should never reach here */
  while (1) {
    bootloader_error_loop();
  }
}

/**
  * @brief  Validate application at APP_START_ADDR
  * @retval BOOTLOADER_OK if application is valid, error code otherwise
  */
static int32_t validate_application(void)
{
  uint32_t app_stack_ptr;
  uint32_t app_reset_handler;
  
  /* Read stack pointer from application vector table */
  app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  
  /* Read reset handler from application vector table */
  app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* Validate stack pointer points to SRAM */
  if (app_stack_ptr < SRAM_BASE || app_stack_ptr > SRAM_END) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* Validate reset handler points to application flash region */
  if (app_reset_handler < APP_START_ADDR || app_reset_handler > APP_END_ADDR) {
    return BOOTLOADER_ERR_INVALID_APP;
  }
  
  /* Simple checksum check (optional, could be enhanced) */
  /* For now, just check if the first location is not all 0xFF or 0x00 */
  if (app_stack_ptr == 0xFFFFFFFF || app_stack_ptr == 0x00000000) {
    return BOOTLOADER_ERR_NO_APP;
  }
  
  return BOOTLOADER_OK;
}

/**
  * @brief  Jump to application at APP_START_ADDR
  * @retval None
  */
static void bootloader_jump_to_app(void)
{
  typedef void (*pFunction)(void);
  
  uint32_t app_stack_ptr;
  uint32_t app_reset_handler;
  pFunction app_entry;
  
  /* Disable all interrupts */
  __disable_irq();
  
  /* Get application stack pointer and reset handler */
  app_stack_ptr = *(volatile uint32_t *)(APP_START_ADDR);
  app_reset_handler = *(volatile uint32_t *)(APP_START_ADDR + 4);
  
  /* Set main stack pointer to application's stack */
  __set_MSP(app_stack_ptr);
  
  /* Relocate vector table to application region */
  SCB->VTOR = APP_START_ADDR;
  
  /* Enable interrupts for application */
  __enable_irq();
  
  /* Jump to application */
  app_entry = (pFunction)(app_reset_handler);
  app_entry();
  
  /* Should never return */
  while (1) {
    /* Error if we get here */
  }
}

/**
  * @brief  Error loop - stays in bootloader mode
  * @retval None
  */
static void bootloader_error_loop(void)
{
  /* Turn LED solid on to indicate error */
  GPIOA->BSRR = (1 << 5);  /* Set PA5 (LED on) */
  
  while (1) {
    /* Stay here - no valid application */
  }
}

/**
  * @brief  Initialize system clocks
  * @retval None
  */
void system_init(void)
{
  /* Reset the RCC clock configuration to the default reset state */
  RCC->CR |= RCC_CR_HSION;                /* Enable HSI */
  while (!(RCC->CR & RCC_CR_HSIRDY));     /* Wait for HSI ready */
  
  RCC->CFGR = 0x00000000;                 /* Reset CFGR register */
  RCC->CR &= ~(RCC_CR_HSEON | RCC_CR_CSSON | RCC_CR_PLLON);  /* Disable HSE, CSS, PLL */
  RCC->CR &= ~RCC_CR_HSEBYP;              /* Disable HSE bypass */
  RCC->CFGR &= ~RCC_CFGR_HPRE;            /* Reset HPRE */
  RCC->CFGR &= ~RCC_CFGR_PPRE1;           /* Reset PPRE1 */
  RCC->CFGR &= ~RCC_CFGR_PPRE2;           /* Reset PPRE2 */
  
  /* Disable all interrupts */
  RCC->CIR = 0x00000000;
  
  /* SystemInit() is called from startup code */
  /* Vector table is already set to FLASH_BASE */
  
  /* Update SystemCoreClock variable */
  SystemCoreClockUpdate();
}

/**
  * @brief  Initialize GPIO for LED (PA5 - built-in LED on many STM32 boards)
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
  * @brief  Simple delay function using SysTick (not calibrated, just busy wait)
  * @param  ms: delay time in milliseconds
  * @retval None
  */
void delay_ms(uint32_t ms)
{
  /* Simple busy-wait delay - not accurate but sufficient for bootloader */
  /* Assuming 16MHz HSI clock (default after reset) */
  volatile uint32_t count;
  volatile uint32_t i;
  
  /* Adjust this value based on actual clock frequency */
  /* For 16MHz: approximately 16000 cycles per millisecond */
  count = ms * 1600;
  
  for (i = 0; i < count; i++) {
    __NOP();
  }
}
