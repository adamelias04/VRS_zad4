/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
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

/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* Memory buffer used directly by DMA for USART Rx*/
uint8_t bufferUSART2dma[DMA_USART2_BUFFER_SIZE];

/* Declaration and initialization of callback function */
static void (* USART2_ProcessData)(uint8_t data) = 0;

/* Register callback */
void USART2_RegisterCallback(void *callback)
{
	if(callback != 0)
	{
		USART2_ProcessData = callback;
	}
}

/* Space for global variables, if you need them */

/* Max. number of bytes the sender transmits at once */
#define USART2_RX_MAX_MSG_LEN 20

/* Position in bufferUSART2dma up to which data was already processed */
static uint16_t old_pos = 0;

/* USART2 init function */
void MX_USART2_UART_Init(void)
{
  LL_USART_InitTypeDef USART_InitStruct = {0};

  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* Peripheral clock enable */
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
  
  LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
  /**USART2 GPIO Configuration  
  PA2   ------> USART2_TX
  PA15   ------> USART2_RX 
  */
  GPIO_InitStruct.Pin = LL_GPIO_PIN_2|LL_GPIO_PIN_15;
  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
  GPIO_InitStruct.Alternate = LL_GPIO_AF_7;
  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*
   * USART2 DMA configuration. Write configuration for DMA used by USART2 for data Rx/Tx with INTERRUPTS.
   * Rx memory buffer will be handled in normal mode, not circular!
   * You can use configuration from example program and modify it.
   * For more information about DMA registers, refer to reference manual.
   */
  
  /* USART2_RX Init */

  	  // type DMA USART Rx configuration here
  LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_6, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
   LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PRIORITY_MEDIUM);
   LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MODE_NORMAL);
   LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PERIPH_NOINCREMENT);
   LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MEMORY_INCREMENT);
   LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_PDATAALIGN_BYTE);
   LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_6, LL_DMA_MDATAALIGN_BYTE);

   LL_DMA_ConfigAddresses(DMA1, LL_DMA_CHANNEL_6,
                          LL_USART_DMA_GetRegAddr(USART2, LL_USART_DMA_REG_DATA_RECEIVE),
                          (uint32_t)bufferUSART2dma,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY);

   LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, DMA_USART2_BUFFER_SIZE);
   LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
   LL_USART_EnableDMAReq_RX(USART2);

   LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_6);
   LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_6);

  /* USART2_TX Init */

	  // type DMA USART Tx configuration here
    LL_DMA_SetDataTransferDirection(DMA1, LL_DMA_CHANNEL_7, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PRIORITY_MEDIUM);
    LL_DMA_SetMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(DMA1, LL_DMA_CHANNEL_7, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(DMA1, LL_DMA_CHANNEL_7, LL_DMA_MDATAALIGN_BYTE);

    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_7, LL_USART_DMA_GetRegAddr(USART2, LL_USART_DMA_REG_DATA_TRANSMIT));
    LL_USART_EnableDMAReq_TX(USART2);

    LL_DMA_EnableIT_TE(DMA1, LL_DMA_CHANNEL_7);

  /* USART2 interrupt Init */
  NVIC_SetPriority(USART2_IRQn, 0);
  NVIC_EnableIRQ(USART2_IRQn);

  USART_InitStruct.BaudRate = 115200;
  USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
  USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
  USART_InitStruct.Parity = LL_USART_PARITY_NONE;
  USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
  USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
  USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
  LL_USART_Init(USART2, &USART_InitStruct);
  LL_USART_ConfigAsyncMode(USART2);
  LL_USART_DisableIT_CTS(USART2);

  /* Enable USART2 peripheral and interrupts*/

  	  //type your code here:
  LL_USART_EnableIT_IDLE(USART2);
  LL_USART_Enable(USART2);
}


// Send data stored in buffer with DMA
void USART2_PutBuffer(uint8_t *buffer, uint8_t length)
{
	LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_7, (uint32_t)buffer);

	LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_7, length);

	LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_7);

	LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_7);
}


/*
 *	Function processing data received via USART2 with DMA and stored in bufferUSART2dma.
 *	Forwards data to callback function.
 *	Keeps track of pointer pointing to Rx memory buffer and resets the pointer if overflow is possible in next Rx.
 *	Refer to reference manual - "normal memory mode" and "increment memory mode".
 */
void USART2_CheckDmaReception(void)
{
	if(USART2_ProcessData == 0) return;

	/* number of bytes DMA has written into the buffer so far */
	uint16_t pos = DMA_USART2_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);

	/* not enough free space for next message -> stop DMA before resetting */
	uint8_t reset = (DMA_USART2_BUFFER_SIZE - pos) < USART2_RX_MAX_MSG_LEN;

	if(reset)
	{
		LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);
		/* a byte may have arrived meanwhile, read position again */
		pos = DMA_USART2_BUFFER_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6);
	}

	/* forward new bytes to callback */
	for(uint16_t i = old_pos; i < pos; i++)
	{
		USART2_ProcessData(bufferUSART2dma[i]);
	}
	old_pos = pos;

	if(reset)
	{
		/* start writing from the beginning of the buffer again */
		LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, DMA_USART2_BUFFER_SIZE);
		LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
		old_pos = 0;
	}
}




/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
