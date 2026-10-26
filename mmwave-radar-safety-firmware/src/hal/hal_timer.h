/**
 * @file hal_timer.h
 * @brief HAL Timer (RTI) Interface
 * @copyright Copyright (c) 2024
 */
#ifndef HAL_TIMER_H
#define HAL_TIMER_H

#include "platform_types.h"

void     HAL_Timer_Init(uint32 timerId, uint32 periodUs);
void     HAL_Timer_Start(uint32 timerId);
void     HAL_Timer_Stop(uint32 timerId);
uint32   HAL_Timer_GetCount(uint32 timerId);
void     HAL_Timer_ClearInterrupt(uint32 timerId);

#endif /* HAL_TIMER_H */
