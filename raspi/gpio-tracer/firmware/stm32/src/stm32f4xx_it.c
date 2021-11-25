
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
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
#include <stm32f4xx_it.h>
#include <stm32f4xx_hal.h>

#include <stdio.h>
/* Private includes ----------------------------------------------------------*/

#include "cc1101.h"
#include "time.h"

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim5;

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  *time_seconds += 1;
  /* pps_int_counter_period = __HAL_TIM_GET_COUNTER(&htim2); */

  uint8_t state = cc1101_get_chip_state();
  if (IS_STATE(state, TX_MODE)) { // if not in TX_MODE (Because auf calibration or tx underflow) send nothing
    /* cc1101_write_tx_fifo((uint8_t*) &time_seconds, 4); */
    cc1101_fast_transmit_current_time();

    while (!(IS_STATE(cc1101_get_chip_state(), IDLE) || IS_STATE(cc1101_get_chip_state(), TXFIFO_UNDERFLOW))) {
      /* HAL_Delay(1U); */
    }
  }

  if (IS_STATE(cc1101_get_chip_state(), TXFIFO_UNDERFLOW)) {
    cc1101_command_strobe(header_command_sftx);
  }

  if (IS_STATE(cc1101_get_chip_state(), IDLE)) {
    cc1101_set_transmit();
  }

  printf("current seconds: %d\n", *time_seconds);

  HAL_TIM_IRQHandler(&htim2);
  /* __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE); // Cause we know that this interrupt will just be triggered by update events */
}

void TIM3_IRQHandler(void)
{
  /* uint8_t state = cc1101_get_chip_state(); */

  /* if (IS_STATE(state, IDLE)) { */
  /*   cc1101_set_transmit(); */
  /* } */

  printf("Setting into receive mode\n");

  HAL_TIM_IRQHandler(&htim3);

  /* __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE); // Cause we know that this interrupt will just be triggered by update events */
}

/**
  * @brief This function handles TIM5 global interrupt.
  */
void TIM5_IRQHandler(void)
{
  // TODO: Maybe handle data transfer via DMA engine
  pps_ext_counter_period = __HAL_TIM_GET_COUNTER(&htim5);

  TIM_HandleTypeDef* htim = &htim5;

  __HAL_TIM_SET_COUNTER(htim, 0);

  uint8_t update_flag = __HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE);
  if (update_flag > 0) {
    pps_measured_pulses = 0;
    /* __HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE); */
    // Handle overflow event here
  } else {
    // else we got two consecutive measured pulses
    pps_measured_pulses += pps_measured_pulses < 2 ? 1 : 0;
  }

  // in case we gathered two consecutive pulses, we acquire a valid estimate for the offset
  if(pps_measured_pulses > 1) {
    __HAL_TIM_SET_AUTORELOAD(&htim2, pps_ext_counter_period);
    printf("valid estimate gathered: %d\n", pps_ext_counter_period);
  }

  HAL_TIM_IRQHandler(&htim5);
}


/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles USB On The Go FS global interrupt.
  */
void OTG_FS_IRQHandler(void)
{
  /* USER CODE BEGIN OTG_FS_IRQn 0 */

  /* USER CODE END OTG_FS_IRQn 0 */
  HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
  /* USER CODE BEGIN OTG_FS_IRQn 1 */

  /* USER CODE END OTG_FS_IRQn 1 */
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
