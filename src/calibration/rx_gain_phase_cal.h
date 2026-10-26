/**
 * @file rx_gain_phase_cal.h
 * @brief RX Gain/Phase Calibration Interface
 * @copyright Copyright (c) 2024
 */
#ifndef RX_GAIN_PHASE_CAL_H
#define RX_GAIN_PHASE_CAL_H

#include "platform_types.h"

Status_t RxGainPhaseCal_Run(void);
Status_t RxGainPhaseCal_GetGainMismatch(uint32 rxCh, float32 *pGaindB);
Status_t RxGainPhaseCal_GetPhaseMismatch(uint32 rxCh, float32 *pPhaseDeg);

#endif /* RX_GAIN_PHASE_CAL_H */
