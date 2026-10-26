/**
 * @file pll_driver.h
 * @brief PLL/Synthesizer Driver Interface
 * @copyright Copyright (c) 2024
 */
#ifndef PLL_DRIVER_H
#define PLL_DRIVER_H

#include "platform_types.h"

Status_t PLL_Init(void);
Status_t PLL_SetFrequency(float32 freqGHz);
Status_t PLL_WaitForLock(uint32 timeoutUs);
boolean  PLL_IsLocked(void);
Status_t PLL_RunCalibration(void);

#endif /* PLL_DRIVER_H */
