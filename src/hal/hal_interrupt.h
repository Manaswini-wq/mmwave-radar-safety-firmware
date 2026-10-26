/**
 * @file hal_interrupt.h
 * @brief HAL Interrupt Controller (VIM) Interface
 * @copyright Copyright (c) 2024
 */
#ifndef HAL_INTERRUPT_H
#define HAL_INTERRUPT_H

#include "platform_types.h"

typedef void (*ISR_Handler_t)(void);

void     HAL_Interrupt_Init(void);
void     HAL_Interrupt_Register(uint32 intNum, ISR_Handler_t handler);
void     HAL_Interrupt_Enable(uint32 intNum);
void     HAL_Interrupt_Disable(uint32 intNum);
void     HAL_Interrupt_GlobalEnable(void);
void     HAL_Interrupt_GlobalDisable(void);
void     HAL_Interrupt_Dispatch(uint32 intNum);

#endif /* HAL_INTERRUPT_H */
