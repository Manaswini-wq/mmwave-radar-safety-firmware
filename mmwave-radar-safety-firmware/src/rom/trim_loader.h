/**
 * @file trim_loader.h
 * @brief Trim/Calibration Parameter Loader Interface
 * @copyright Copyright (c) 2024
 */
#ifndef TRIM_LOADER_H
#define TRIM_LOADER_H

#include "platform_types.h"

Status_t TrimLoader_LoadAll(void);
Status_t TrimLoader_GetTxTrim(uint32 txCh, uint32 *pTrimVal);
Status_t TrimLoader_GetRxTrim(uint32 rxCh, uint32 *pTrimVal);

#endif /* TRIM_LOADER_H */
