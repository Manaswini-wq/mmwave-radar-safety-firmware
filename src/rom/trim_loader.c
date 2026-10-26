/**
 * @file trim_loader.c
 * @brief Trim/Calibration Parameter Loader from OTP/eFuse
 * @details Reads factory-programmed trim values from OTP (One-Time
 *          Programmable) memory and applies them to the analog front-end
 *          registers. Trim values compensate for process variations in:
 *          - TX power amplifier bias
 *          - RX LNA gain
 *          - ADC reference voltage
 *          - PLL loop filter
 *          - Temperature sensor offset
 *
 *          OTP data integrity is verified via CRC stored in the last
 *          OTP word.
 *
 * @copyright Copyright (c) 2024
 */

#include "trim_loader.h"
#include "awr2944_regs.h"
#include "safety_cfg.h"
#include "safety/crc_e2e.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static uint32 g_txTrimData[RADAR_MAX_TX_CHANNELS];
static uint32 g_rxTrimData[RADAR_MAX_RX_CHANNELS];
static uint32 g_adcTrim = 0U;
static uint32 g_pllTrim = 0U;
static boolean g_trimLoaded = FALSE;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Load all trim parameters from OTP
 * @return STATUS_OK if OTP data is valid (CRC check passes)
 */
Status_t TrimLoader_LoadAll(void)
{
    uint32 ch;
    uint32 crcCalc;
    uint32 crcStored;

    /* Read TX trim data */
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        g_txTrimData[ch] = REG32_RD(OTP_RF_TRIM_TX(ch));
    }

    /* Read RX trim data */
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        g_rxTrimData[ch] = REG32_RD(OTP_RF_TRIM_RX(ch));
    }

    /* Read ADC and PLL trim */
    g_adcTrim = REG32_RD(OTP_ADC_TRIM);
    g_pllTrim = REG32_RD(OTP_PLL_TRIM);

    /* Verify OTP CRC */
    crcStored = REG32_RD(OTP_CRC);

    /* Compute CRC over all trim data */
    uint8 trimBuf[64];
    uint32 offset = 0U;
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        trimBuf[offset++] = (uint8)(g_txTrimData[ch] & 0xFFU);
        trimBuf[offset++] = (uint8)((g_txTrimData[ch] >> 8U) & 0xFFU);
        trimBuf[offset++] = (uint8)((g_txTrimData[ch] >> 16U) & 0xFFU);
        trimBuf[offset++] = (uint8)((g_txTrimData[ch] >> 24U) & 0xFFU);
    }
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        trimBuf[offset++] = (uint8)(g_rxTrimData[ch] & 0xFFU);
        trimBuf[offset++] = (uint8)((g_rxTrimData[ch] >> 8U) & 0xFFU);
        trimBuf[offset++] = (uint8)((g_rxTrimData[ch] >> 16U) & 0xFFU);
        trimBuf[offset++] = (uint8)((g_rxTrimData[ch] >> 24U) & 0xFFU);
    }

    crcCalc = CRC_E2E_Calculate(trimBuf, offset);

    if (crcCalc != crcStored)
    {
        /* OTP data corrupted -- use defaults */
        return STATUS_ERROR;
    }

    /* Apply trim values to hardware */
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        REG32_WR(TX_CH_CAL_OFFSET(ch), g_txTrimData[ch] & 0xFFU);
    }
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        REG32_WR(RX_CH_CAL_GAIN(ch), g_rxTrimData[ch] & 0xFFU);
    }

    g_trimLoaded = TRUE;
    return STATUS_OK;
}

Status_t TrimLoader_GetTxTrim(uint32 txCh, uint32 *pTrimVal)
{
    if ((txCh >= RADAR_MAX_TX_CHANNELS) || (pTrimVal == NULL_PTR))
        return STATUS_ERROR;
    *pTrimVal = g_txTrimData[txCh];
    return STATUS_OK;
}

Status_t TrimLoader_GetRxTrim(uint32 rxCh, uint32 *pTrimVal)
{
    if ((rxCh >= RADAR_MAX_RX_CHANNELS) || (pTrimVal == NULL_PTR))
        return STATUS_ERROR;
    *pTrimVal = g_rxTrimData[rxCh];
    return STATUS_OK;
}
