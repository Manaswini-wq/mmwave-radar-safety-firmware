/**
 * @file tx_power_cal.c
 * @brief TX Power Calibration Algorithm
 * @details Measures TX output power via internal loopback path and
 *          computes per-channel power correction factors for flatness
 *          across the chirp bandwidth. Calibration data is applied
 *          to the TX power DAC registers.
 *
 *          Algorithm:
 *          1. Enable internal TX-to-RX loopback
 *          2. Transmit CW tone at target power
 *          3. Measure received power via ADC
 *          4. Compute correction = target_power - measured_power
 *          5. Apply correction to TX power register
 *          6. Iterate until convergence (<0.5 dB error)
 *
 * @copyright Copyright (c) 2024
 */

#include "tx_power_cal.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief Per-channel power correction in 0.1 dB units */
static int32  g_txPowerCorrection[RADAR_MAX_TX_CHANNELS];
static boolean g_txPowerCalDone = FALSE;

/* Calibration parameters */
#define TX_CAL_TARGET_POWER_DBM     120   /* 12.0 dBm in 0.1 dB units */
#define TX_CAL_MAX_ITERATIONS       10U
#define TX_CAL_CONVERGENCE_THRESH   5     /* 0.5 dB in 0.1 dB units */
#define TX_CAL_LOOPBACK_LOSS_DB     30    /* 3.0 dB loopback attenuation */

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static int32 TxPowerCal_MeasurePower(uint32 txChannel);
static void  TxPowerCal_ApplyCorrection(uint32 txChannel, int32 correction);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Run TX power calibration on all channels
 * @return STATUS_OK if calibration converges for all channels
 */
Status_t TxPowerCal_Run(void)
{
    uint32 ch;
    uint32 iter;
    boolean allConverged;

    /* Initialize corrections to zero */
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        g_txPowerCorrection[ch] = 0;
    }

    /* Enable internal loopback path */
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        REG32_WR(TX_CH_LOOPBACK(ch), 0x01U);
    }

    /* Iterative calibration loop */
    for (iter = 0U; iter < TX_CAL_MAX_ITERATIONS; iter++)
    {
        allConverged = TRUE;

        for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
        {
            /* Measure current TX power via loopback */
            int32 measuredPower = TxPowerCal_MeasurePower(ch);

            /* Account for loopback path loss */
            measuredPower += TX_CAL_LOOPBACK_LOSS_DB;

            /* Compute error */
            int32 error = TX_CAL_TARGET_POWER_DBM - measuredPower;

            if ((error > TX_CAL_CONVERGENCE_THRESH) ||
                (error < -TX_CAL_CONVERGENCE_THRESH))
            {
                allConverged = FALSE;
            }

            /* Update correction (use 50% of error for stability) */
            g_txPowerCorrection[ch] += (error / 2);

            /* Apply correction to hardware */
            TxPowerCal_ApplyCorrection(ch, g_txPowerCorrection[ch]);
        }

        if (allConverged == TRUE)
        {
            break;
        }
    }

    /* Disable loopback */
    for (ch = 0U; ch < RADAR_MAX_TX_CHANNELS; ch++)
    {
        REG32_WR(TX_CH_LOOPBACK(ch), 0x00U);
    }

    g_txPowerCalDone = TRUE;

    return allConverged ? STATUS_OK : STATUS_ERROR;
}

/**
 * @brief Get the calibration correction for a TX channel
 */
Status_t TxPowerCal_GetCorrection(uint32 txChannel, int32 *pCorrectiondB)
{
    if ((txChannel >= RADAR_MAX_TX_CHANNELS) || (pCorrectiondB == NULL_PTR))
    {
        return STATUS_ERROR;
    }

    *pCorrectiondB = g_txPowerCorrection[txChannel];
    return STATUS_OK;
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

/**
 * @brief Measure TX power on a channel via loopback ADC capture
 * @param txChannel TX channel index
 * @return Measured power in 0.1 dB units
 */
static int32 TxPowerCal_MeasurePower(uint32 txChannel)
{
    uint32 adcSum = 0U;
    uint32 numSamples = 64U;
    uint32 i;

    /* Enable only the target TX channel */
    for (i = 0U; i < RADAR_MAX_TX_CHANNELS; i++)
    {
        if (i == txChannel)
        {
            REG32_WR(TX_CH_CTRL(i), TX_CH_EN_MASK);
        }
        else
        {
            REG32_WR(TX_CH_CTRL(i), 0x00U);
        }
    }

    /* Trigger ADC capture */
    REG32_SET(ADC_CTRL, ADC_CTRL_ENABLE);

    /* Wait for data */
    volatile uint32 timeout = 100000U;
    while (((REG32_RD(ADC_INT_STATUS) & ADC_INT_BUFFER_FULL) == 0U) &&
           (timeout > 0U))
    {
        timeout--;
    }

    /* Read and accumulate ADC samples */
    for (i = 0U; i < numSamples; i++)
    {
        uint32 sample = REG32_RD(ADC_BUFFER_BASE + (i * 4U)) & 0x0FFFU;
        adcSum += sample;
    }

    /* Clear ADC interrupt */
    REG32_WR(ADC_INT_CLEAR, ADC_INT_BUFFER_FULL);

    /* Convert ADC average to power in 0.1 dB units */
    /* Simplified: P(dB) = 20*log10(Vavg/Vref) * 10 */
    uint32 avg = adcSum / numSamples;

    /* Linear approximation: power ~= (avg - 2048) * 0.05 dBFS
     * Offset to absolute dBm using reference calibration */
    int32 powerTenthDb = (int32)((avg * 10U) / 41U);  /* Simplified scaling */

    return powerTenthDb;
}

/**
 * @brief Apply power correction to TX channel hardware register
 */
static void TxPowerCal_ApplyCorrection(uint32 txChannel, int32 correction)
{
    /* Convert 0.1 dB correction to register code */
    /* TX power register: 5-bit unsigned, 1 LSB = 1 dB */
    int32 regVal = (int32)REG32_RD(TX_CH_POWER(txChannel)) & 0x1FU;
    regVal += (correction / 10);

    /* Clamp to valid range */
    if (regVal < 0)
    {
        regVal = 0;
    }
    if (regVal > (int32)TX_CH_POWER_MAX)
    {
        regVal = (int32)TX_CH_POWER_MAX;
    }

    REG32_WR(TX_CH_POWER(txChannel), (uint32)regVal);

    /* Also write fine correction to calibration offset register */
    REG32_WR(TX_CH_CAL_OFFSET(txChannel), (uint32)(correction & 0xFFU));
}
