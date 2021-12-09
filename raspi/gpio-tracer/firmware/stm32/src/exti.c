#include "exti.h"
#include "main.h"

#include <stdio.h>

volatile char triggered = 0;

void InitializeExtItSrc();

void MX_EXTI_Init(void) {
  /* __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_3); */

  InitializeExtItSrc();

  /* EXTI_ConfigTypeDef EXTI_InitStruct = {0}; */

  /* EXTI_InitStruct.Line = EXTI_LINE_11; */
  /* EXTI_InitStruct.Mode = EXTI_MODE_INTERRUPT; */
  /* EXTI_InitStruct.Trigger = EXTI_TRIGGER_RISING; */
  /* EXTI_InitStruct.GPIOSel = EXTI_GPIOA; */

  HAL_NVIC_SetPriority(EXTI3_IRQn, 2, 0);

  HAL_NVIC_EnableIRQ(EXTI3_IRQn);
}

/**
  * @brief Initialize external interrupt source for cc1101 package arrival signal.
  * @retval None
  */
void InitializeExtItSrc() {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin = GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  /* GPIO_InitStruct.Speed = GPIO_SPEED_OW; */
  /* GPIO_InitStruct.Alternate = G; */

  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_3)
    {
      printf("Interrupt detected\n");
    }
}
