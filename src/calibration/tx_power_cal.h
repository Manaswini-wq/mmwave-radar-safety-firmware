/**
 * @file tx_power_cal.h
 * @brief TX Power Calibration Interface
 * @copyright Copyright (c) 2024
 */
#ifndef TX_POWER_CAL_H
#define TX_POWER_CAL_H

#include "platform_types.h"

Status_t TxPowerCal_Run(void);
Status_t TxPowerCal_GetCorrection(uint32 txChannel, int32 *pCorrectiondB);

#endif /* TX_POWER_CAL_H */
