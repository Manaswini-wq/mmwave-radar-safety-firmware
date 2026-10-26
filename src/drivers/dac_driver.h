/**
 * @file dac_driver.h
 * @brief DAC Driver Interface for AWR2944
 * @copyright Copyright (c) 2024
 */
#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include "platform_types.h"

Status_t DAC_Init(void);
Status_t DAC_SetOutput(uint32 value);
Status_t DAC_SetGain(uint32 gainCode);
Status_t DAC_SetOffset(int32 offsetCode);
Status_t DAC_Enable(void);
Status_t DAC_Disable(void);

#endif /* DAC_DRIVER_H */
