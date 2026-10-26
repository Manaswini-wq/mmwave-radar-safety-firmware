/**
 * @file hal_dma.c
 * @brief HAL DMA Engine Implementation
 * @details Manages the DMA controller for high-speed data transfers
 *          between ADC buffers and processing memory. Supports
 *          channel chaining for ping-pong buffer operation.
 *
 * @copyright Copyright (c) 2024
 */

#include "hal_dma.h"
#include "awr2944_regs.h"

/**
 * @brief Initialize DMA controller
 */
void HAL_DMA_Init(void)
{
    /* Reset DMA */
    REG32_WR(DMA_CTRL, 0x01U);  /* Software reset */
    volatile uint32 d;
    for (d = 0U; d < 1000U; d++) { /* spin */ }
    REG32_WR(DMA_CTRL, 0x00U);

    /* Disable all DMA channels */
    uint32 ch;
    for (ch = 0U; ch < DMA_MAX_CHANNELS; ch++)
    {
        REG32_WR(DMA_CH_CTRL(ch), 0x00U);
    }

    /* Clear all interrupt status */
    REG32_WR(DMA_INT_CLEAR, 0xFFFFFFFFU);

    /* Enable DMA controller */
    REG32_WR(DMA_CTRL, 0x02U);  /* Enable bit */
}

/**
 * @brief Configure a DMA channel
 */
Status_t HAL_DMA_Configure(uint32 channel, uint32 srcAddr, uint32 dstAddr, uint32 size)
{
    if (channel >= DMA_MAX_CHANNELS) return STATUS_ERROR;

    REG32_WR(DMA_CH_SRC_ADDR(channel), srcAddr);
    REG32_WR(DMA_CH_DST_ADDR(channel), dstAddr);
    REG32_WR(DMA_CH_XFER_SIZE(channel), size);

    return STATUS_OK;
}

/**
 * @brief Start a DMA transfer
 */
Status_t HAL_DMA_Start(uint32 channel)
{
    if (channel >= DMA_MAX_CHANNELS) return STATUS_ERROR;

    REG32_WR(DMA_CH_CTRL(channel),
             DMA_CH_CTRL_ENABLE | DMA_CH_CTRL_TRIGGER | DMA_CH_CTRL_INT_EN);
    return STATUS_OK;
}

/**
 * @brief Stop a DMA channel
 */
Status_t HAL_DMA_Stop(uint32 channel)
{
    if (channel >= DMA_MAX_CHANNELS) return STATUS_ERROR;
    REG32_WR(DMA_CH_CTRL(channel), 0x00U);
    return STATUS_OK;
}

/**
 * @brief Check if DMA transfer is complete
 */
boolean HAL_DMA_IsDone(uint32 channel)
{
    if (channel >= DMA_MAX_CHANNELS) return FALSE;
    return ((REG32_RD(DMA_CH_STATUS(channel)) & 0x01U) != 0U) ? TRUE : FALSE;
}
