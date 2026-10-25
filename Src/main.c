/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);


/* Function processing DMA Rx data. Counts how many capital and small letters are in sentence.
 * Result is supposed to be stored in global variable of type "letter_count_" that is defined in "main.h"
 *
 * @param1 - received sign
 */
void proccesDmaData(uint8_t sign);


/* Space for your global variables. */

/* Max. number of signs after '#' within which '$' has to be received */
#define MAX_STRING_LEN 35

/* Result of last valid string - amount of small and capital letters */
letter_count_ letter_count = {0};

/* Last valid string (without '#' and '$') */
char valid_string[MAX_STRING_LEN];

/* Set in proccesDmaData when new valid string is received, cleared in main loop */
volatile uint8_t valid_string_ready = 0;

/* Period of DMA Rx buffer state transmission - 0.5 Hz */
#define BUFFER_STATE_PERIOD_MS 2000

volatile uint32_t tick_ms = 0;

/* Buffer for messages transmitted via USART2 */
char tx_msg[100];

/* Wait until previous DMA transmission is finished */
static void wait_tx_done(void)
{
	while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_7));
}


int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();

  /* Space for your local variables, callback registration ...*/

  	  //type your code here:
  USART2_RegisterCallback(proccesDmaData);

  /* SysTick is already set to 1 ms by LL_Init1msTick, enable its interrupt for tick_ms */
  LL_SYSTICK_EnableIT();
  uint32_t last_state_tick = 0;

  while (1)
  {
	  /* Periodic transmission of information about DMA Rx buffer state.
	   * Transmission frequency - 0.5Hz.
	   * Message format - "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %]: %f%"
	   * Example message (what I wish to see in terminal) - Buffer capacity: 1000 bytes, occupied memory: 231 bytes, load [in %]: 23.1%
	   */

	  /* Valid text string information transmission.
	   * Transmission frequency - when new valid string is received.
	   * Message format - "Valid string: %s, lower-case: %d, upper-case: %d"
	   * Example message (what I wish to see in terminal) - Valid string: Platn15uborZnakov, lower-case: 13, upper-case: 2
	   */

  	  	  	  //type your code here:
	  if(tick_ms - last_state_tick >= BUFFER_STATE_PERIOD_MS)
	  {
		  last_state_tick += BUFFER_STATE_PERIOD_MS;

		  uint16_t occupied = DMA_USART2_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
		  /* load in tenths of percent, rounded - printf without float support */
		  uint16_t load = (occupied * 1000 + DMA_USART2_BUFFER_SIZE / 2) / DMA_USART2_BUFFER_SIZE;

		  wait_tx_done();
		  int len = sprintf(tx_msg, "Buffer capacity: %d bytes, occupied memory: %d bytes, load [in %%]: %d.%d%%\r\n",
				  DMA_USART2_BUFFER_SIZE, occupied, load / 10, load % 10);
		  USART2_PutBuffer((uint8_t*)tx_msg, len);
	  }

	  if(valid_string_ready)
	  {
		  wait_tx_done();

		  /* proccesDmaData runs in interrupt - don't let it change data while formatting */
		  __disable_irq();
		  int len = sprintf(tx_msg, "Valid string: %s, lower-case: %d, upper-case: %d\r\n",
				  valid_string, letter_count.small_letter, letter_count.capital_letter);
		  valid_string_ready = 0;
		  __enable_irq();

		  USART2_PutBuffer((uint8_t*)tx_msg, len);
	  }
  }
  /* USER CODE END 3 */
}


void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);

  if(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  Error_Handler();  
  }
  LL_RCC_HSI_Enable();

   /* Wait till HSI is ready */
  while(LL_RCC_HSI_IsReady() != 1)
  {

  }
  LL_RCC_HSI_SetCalibTrimming(16);
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSI);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSI)
  {
  
  }
  LL_Init1msTick(8000000);
  LL_SYSTICK_SetClkSource(LL_SYSTICK_CLKSOURCE_HCLK);
  LL_SetSystemCoreClock(8000000);
}

/*
 * Implementation of function processing data received via USART.
 */
void proccesDmaData(uint8_t sign)
{
	/* Process received data */
	static uint8_t receiving = 0;			/* '#' was received, waiting for '$' */
	static uint8_t len = 0;					/* signs received after '#' */
	static char string[MAX_STRING_LEN];
	static letter_count_ count = {0};

	if(sign == '#')
	{
		/* start (or restart) of new string */
		receiving = 1;
		len = 0;
		count.small_letter = 0;
		count.capital_letter = 0;
		return;
	}

	if(!receiving) return;

	len++;

	if(sign == '$')
	{
		/* valid string - publish result for main loop */
		string[len - 1] = '\0';
		for(uint8_t i = 0; i < len; i++)
		{
			valid_string[i] = string[i];
		}
		letter_count = count;
		valid_string_ready = 1;
		receiving = 0;
		return;
	}

	if(len >= MAX_STRING_LEN)
	{
		/* '$' not received within 35 signs - discard data */
		receiving = 0;
		return;
	}

	string[len - 1] = sign;

	if(sign >= 'a' && sign <= 'z')
	{
		count.small_letter++;
	}
	else if(sign >= 'A' && sign <= 'Z')
	{
		count.capital_letter++;
	}
}


void Error_Handler(void)
{

}

#ifdef  USE_FULL_ASSERT

void assert_failed(char *file, uint32_t line)
{ 

}

#endif

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
