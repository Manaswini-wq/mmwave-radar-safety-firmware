/**
 * @file spi_driver.c
 * @brief SPI Driver for Firmware-Hardware Interface Communication
 * @details Implements SPI master mode for communication with the host
 *          processor (e.g., external MCU or application processor).
 *          Supports polled and interrupt-driven transfer modes.
 *
 * @copyright Copyright (c) 2024
 */

#include "spi_driver.h"
#include "awr2944_regs.h"

static boolean g_spiInitialized = FALSE;

/**
 * @brief Initialize SPI peripheral in master mode
 */
Status_t SPI_Init(void)
{
    /* Reset SPI */
    REG32_CLR(SPI_CTRL0, SPI_CTRL_ENABLE);

    /* Configure: master mode, CPOL=0, CPHA=0, 8-bit */
    REG32_WR(SPI_CTRL0, SPI_CTRL_MASTER);

    /* Set clock divider for 10 MHz SPI clock (200 MHz / 20) */
    REG32_WR(SPI_CLK_DIV, 20U);

    /* Configure chip select (active low, CS0) */
    REG32_WR(SPI_CS_CTRL, 0x01U);

    /* Clear interrupt status */
    REG32_WR(SPI_INT_CLEAR, 0xFFFFFFFFU);

    /* Enable SPI */
    REG32_SET(SPI_CTRL0, SPI_CTRL_ENABLE);

    g_spiInitialized = TRUE;
    return STATUS_OK;
}

/**
 * @brief Transmit a single byte
 */
Status_t SPI_TransmitByte(uint8 data)
{
    if (g_spiInitialized != TRUE) return STATUS_NOT_READY;

    /* Wait for TX FIFO not full */
    volatile uint32 timeout = 10000U;
    while (((REG32_RD(SPI_STATUS) & SPI_STATUS_TX_EMPTY) == 0U) && (timeout > 0U))
    {
        timeout--;
    }
    if (timeout == 0U) return STATUS_TIMEOUT;

    REG32_WR(SPI_DATA, (uint32)data);

    /* Wait for transfer complete */
    timeout = 10000U;
    while ((SPI_IsBusy() == TRUE) && (timeout > 0U))
    {
        timeout--;
    }

    return STATUS_OK;
}

/**
 * @brief Receive a single byte
 */
Status_t SPI_ReceiveByte(uint8 *pData)
{
    if ((g_spiInitialized != TRUE) || (pData == NULL_PTR))
    {
        return STATUS_ERROR;
    }

    /* Send dummy byte to generate clock */
    REG32_WR(SPI_DATA, 0xFFU);

    volatile uint32 timeout = 10000U;
    while (((REG32_RD(SPI_STATUS) & SPI_STATUS_RX_FULL) == 0U) && (timeout > 0U))
    {
        timeout--;
    }
    if (timeout == 0U) return STATUS_TIMEOUT;

    *pData = (uint8)(REG32_RD(SPI_DATA) & 0xFFU);
    return STATUS_OK;
}

/**
 * @brief Full-duplex SPI transfer
 */
Status_t SPI_Transfer(const uint8 *pTxBuf, uint8 *pRxBuf, uint32 length)
{
    uint32 i;

    if (g_spiInitialized != TRUE) return STATUS_NOT_READY;

    for (i = 0U; i < length; i++)
    {
        uint8 txByte = (pTxBuf != NULL_PTR) ? pTxBuf[i] : 0xFFU;

        /* Wait TX ready */
        volatile uint32 timeout = 10000U;
        while (((REG32_RD(SPI_STATUS) & SPI_STATUS_TX_EMPTY) == 0U) && (timeout > 0U))
        {
            timeout--;
        }
        if (timeout == 0U) return STATUS_TIMEOUT;

        REG32_WR(SPI_DATA, (uint32)txByte);

        /* Wait for completion */
        timeout = 10000U;
        while ((SPI_IsBusy() == TRUE) && (timeout > 0U))
        {
            timeout--;
        }

        if (pRxBuf != NULL_PTR)
        {
            pRxBuf[i] = (uint8)(REG32_RD(SPI_DATA) & 0xFFU);
        }
    }

    return STATUS_OK;
}

/**
 * @brief Check if SPI is busy
 */
boolean SPI_IsBusy(void)
{
    return ((REG32_RD(SPI_STATUS) & SPI_STATUS_BUSY) != 0U) ? TRUE : FALSE;
}
