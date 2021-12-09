#ifndef EXTI_H
#define EXTI_H

#include <stm32f4xx_hal.h>

extern volatile char triggered;

void MX_EXTI_Init(void);

#endif /* EXTI_H */
