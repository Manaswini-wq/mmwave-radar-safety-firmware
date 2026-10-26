/**
 * @file data_acquisition.c
 * @brief High-Speed Data Acquisition Implementation
 * @details Manages DMA-based ADC data capture using ping-pong double
 *          buffering. While one buffer receives DMA data, the other
 *          is available for processing. Each completed buffer is
 *          protected with E2E CRC before host transfer.
 *
 *          Data flow: ADC -> DMA -> Ping/Pong Buffer -> CRC -> Host
 *
 * @copyright Copyright (c) 2024
 */

#include "data_acquisition.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"
#include "hal/hal_dma.h"
#include "drivers/spi_driver.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief Ping-pong DMA buffers placed in L3 shared RAM */
static uint32 g_pingBuffer[DATA_ACQ_BUFFER_SIZE / 4U]
    __attribute__((section(".dma_buffers"), aligned(128)));
static uint32 g_pongBuffer[DATA_ACQ_BUFFER_SIZE / 4U]
    __attribute__((section(".dma_buffers"), aligned(128)));

/** @brief Buffer descriptors */
static DataAcq_Buffer_t g_buffers[2];

/** @brief Active buffer index (0 = ping receiving DMA, 1 = pong) */
static uint32 g_activeIdx = 0U;

/** @brief DMA channel used for ADC capture */
#define ADC_DMA_CHANNEL   0U

static boolean g_dataAcqInit = FALSE;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize data acquisition subsystem
 * @details Configures ping-pong buffers, sets up DMA channel for ADC
 *          data capture, and enables DMA chaining for continuous capture.
 * @return STATUS_OK on success
 */
Status_t DataAcq_Init(void)
{
    /* Initialize buffer descriptors */
    g_buffers[0].bufferIdx = 0U;
    g_buffers[0].pData     = g_pingBuffer;
    g_buffers[0].dataSize  = 0U;
    g_buffers[0].ready     = FALSE;

    g_buffers[1].bufferIdx = 1U;
    g_buffers[1].pData     = g_pongBuffer;
    g_buffers[1].dataSize  = 0U;
    g_buffers[1].ready     = FALSE;

    g_activeIdx = 0U;

    /* Zero-fill buffers (required for ECC on L3 RAM) */
    uint32 i;
    for (i = 0U; i < (DATA_ACQ_BUFFER_SIZE / 4U); i++)
    {
        g_pingBuffer[i] = 0U;
        g_pongBuffer[i] = 0U;
    }

    /* Configure DMA channel for ADC -> Ping buffer */
    REG32_WR(DMA_CH_SRC_ADDR(ADC_DMA_CHANNEL), ADC_BUFFER_BASE);
    REG32_WR(DMA_CH_DST_ADDR(ADC_DMA_CHANNEL), (uint32)g_pingBuffer);
    REG32_WR(DMA_CH_XFER_SIZE(ADC_DMA_CHANNEL), DATA_ACQ_BUFFER_SIZE);

    /* Enable DMA interrupt on completion and auto-chain to other buffer */
    REG32_WR(DMA_CH_CTRL(ADC_DMA_CHANNEL),
             DMA_CH_CTRL_ENABLE | DMA_CH_CTRL_INT_EN | DMA_CH_CTRL_CHAIN_EN);

    /* Configure ADC to trigger DMA on buffer full */
    REG32_WR(ADC_DMA_TRIGGER, 0x01U);
    REG32_WR(ADC_INT_ENABLE, ADC_INT_BUFFER_FULL);

    g_dataAcqInit = TRUE;

    return STATUS_OK;
}

/**
 * @brief Get the buffer that is ready for processing
 * @return Pointer to ready buffer, or NULL if none ready
 */
DataAcq_Buffer_t *DataAcq_GetReadyBuffer(void)
{
    /* The non-active buffer should be the ready one */
    uint32 readyIdx = (g_activeIdx == 0U) ? 1U : 0U;

    if (g_buffers[readyIdx].ready == TRUE)
    {
        return &g_buffers[readyIdx];
    }

    return NULL_PTR;
}

/**
 * @brief Validate E2E CRC on a captured data buffer
 * @param pBuffer Pointer to buffer to validate
 * @return STATUS_OK if CRC is valid
 */
Status_t DataAcq_ValidateCRC(const DataAcq_Buffer_t *pBuffer)
{
    if (pBuffer == NULL_PTR)
    {
        return STATUS_ERROR;
    }

    return CRC_E2E_Check(&pBuffer->e2eHeader,
                          (const uint8 *)pBuffer->pData,
                          pBuffer->dataSize);
}

/**
 * @brief Queue a validated buffer for host transfer
 * @param pBuffer Pointer to validated buffer
 */
void DataAcq_QueueForTransfer(DataAcq_Buffer_t *pBuffer)
{
    if (pBuffer == NULL_PTR)
    {
        return;
    }

    /* In a real system, this would queue the buffer for SPI/mailbox DMA
     * transfer to the host processor. For now, mark as consumed. */
    pBuffer->ready = FALSE;
}

/**
 * @brief Swap active DMA buffer (called from chirp-done ISR)
 * @details Switches DMA destination between ping and pong buffers.
 *          Computes E2E CRC on the just-completed buffer.
 */
void DataAcq_SwapBuffers(void)
{
    uint32 completedIdx = g_activeIdx;

    /* Mark completed buffer as ready */
    g_buffers[completedIdx].dataSize = DATA_ACQ_BUFFER_SIZE;
    g_buffers[completedIdx].ready    = TRUE;

    /* Compute E2E CRC on completed buffer */
    CRC_E2E_Protect(&g_buffers[completedIdx].e2eHeader,
                     (const uint8 *)g_buffers[completedIdx].pData,
                     g_buffers[completedIdx].dataSize);

    /* Switch to other buffer */
    g_activeIdx = (g_activeIdx == 0U) ? 1U : 0U;

    /* Reprogram DMA for new active buffer */
    REG32_WR(DMA_CH_DST_ADDR(ADC_DMA_CHANNEL),
             (uint32)g_buffers[g_activeIdx].pData);
    REG32_WR(DMA_CH_XFER_SIZE(ADC_DMA_CHANNEL), DATA_ACQ_BUFFER_SIZE);
    REG32_WR(DMA_CH_CTRL(ADC_DMA_CHANNEL),
             DMA_CH_CTRL_ENABLE | DMA_CH_CTRL_INT_EN | DMA_CH_CTRL_CHAIN_EN);
}

/**
 * @brief Notify host that frame data is available
 * @details Sends a notification via SPI mailbox indicating a complete
 *          frame is ready for host readout.
 */
void DataAcq_NotifyHost(void)
{
    /* Write notification to mailbox */
    uint32 notification = 0x46524D45U;  /* "FRME" */
    REG32_WR(0x50800000U, notification);  /* Mailbox base */

    /* Optionally trigger SPI interrupt to host */
    SPI_TransmitByte(0xAAU);  /* Frame-ready notification byte */
}
