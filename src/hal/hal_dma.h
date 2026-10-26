/**
 * @file hal_dma.h
 * @brief HAL DMA Engine Interface
 * @copyright Copyright (c) 2024
 */
#ifndef HAL_DMA_H
#define HAL_DMA_H

#include "platform_types.h"

void     HAL_DMA_Init(void);
Status_t HAL_DMA_Configure(uint32 channel, uint32 srcAddr, uint32 dstAddr, uint32 size);
Status_t HAL_DMA_Start(uint32 channel);
Status_t HAL_DMA_Stop(uint32 channel);
boolean  HAL_DMA_IsDone(uint32 channel);

#endif /* HAL_DMA_H */
