/**
 * @file lo_leakage_cal.h
 * @brief LO Leakage Calibration Interface
 * @copyright Copyright (c) 2024
 */
#ifndef LO_LEAKAGE_CAL_H
#define LO_LEAKAGE_CAL_H

#include "platform_types.h"

Status_t LOLeakageCal_Run(void);
Status_t LOLeakageCal_GetResidual(int32 *pResidualI, int32 *pResidualQ);

#endif /* LO_LEAKAGE_CAL_H */
