/**
 * @file pll_driver.c
 * @brief PLL/Synthesizer Driver for AWR2944
 * @details Controls the on-chip frequency synthesizer used to generate
 *          the chirp signal for FMCW radar operation. Programs frequency
 *          dividers, loop bandwidth, charge pump current, and VCO
 *          calibration. Provides lock detection and slip monitoring.
 *
 * @copyright Copyright (c) 2024
 */

#include "pll_driver.h"
#include "awr2944_regs.h"
#include "safety_cfg.h"

static boolean g_pllInitialized = FALSE;
static float32 g_currentFreqGHz = 77.0f;

#define PLL_REF_CLK_MHZ   40U
#define PLL_VCO_MIN_GHZ   76.0f
#define PLL_VCO_MAX_GHZ   81.0f

/**
 * @brief Initialize PLL subsystem
 */
Status_t PLL_Init(void)
{
    /* Configure PLL for 77 GHz default */
    REG32_WR(PLL_CTRL, 0x00U);  /* Disable PLL during config */

    /* Set reference divider (R) */
    REG32_WR(PLL_DIVIDER_R, 1U);  /* Divide-by-1 */

    /* Set feedback divider (N) for 77 GHz */
    /* f_VCO = f_REF * N / R = 40 MHz * 1925 = 77 GHz */
    REG32_WR(PLL_DIVIDER_N, 1925U);

    /* Configure loop bandwidth */
    REG32_WR(PLL_LOOP_BW, 0x05U);  /* ~500 kHz loop BW */

    /* Set charge pump current */
    REG32_WR(PLL_CHARGE_PUMP, 0x08U);  /* Nominal ICP */

    /* VCO control */
    REG32_WR(PLL_VCO_CTRL, 0x03U);  /* Auto VCO band selection */

    /* Compute and set frequency word */
    uint32 freqWord = (uint32)((77.0f / 40.0f) * 67108864.0f);
    REG32_WR(PLL_FREQ_WORD_L, freqWord & 0xFFFFU);
    REG32_WR(PLL_FREQ_WORD_H, (freqWord >> 16U) & 0xFFFFU);

    /* Enable PLL */
    REG32_WR(PLL_CTRL, PLL_CTRL_ENABLE);

    /* Clear any pending status */
    REG32_WR(PLL_STATUS, PLL_STATUS_SLIP);

    g_currentFreqGHz = 77.0f;
    g_pllInitialized = TRUE;

    return STATUS_OK;
}

/**
 * @brief Set PLL output frequency
 * @param freqGHz Target frequency in GHz
 * @return STATUS_OK on success
 */
Status_t PLL_SetFrequency(float32 freqGHz)
{
    if (g_pllInitialized != TRUE) return STATUS_NOT_READY;
    if ((freqGHz < PLL_VCO_MIN_GHZ) || (freqGHz > PLL_VCO_MAX_GHZ))
    {
        return STATUS_ERROR;
    }

    uint32 divN = (uint32)((freqGHz * 1000.0f) / (float32)PLL_REF_CLK_MHZ);
    REG32_WR(PLL_DIVIDER_N, divN);

    uint32 freqWord = (uint32)((freqGHz / 40.0f) * 67108864.0f);
    REG32_WR(PLL_FREQ_WORD_L, freqWord & 0xFFFFU);
    REG32_WR(PLL_FREQ_WORD_H, (freqWord >> 16U) & 0xFFFFU);

    g_currentFreqGHz = freqGHz;

    return PLL_WaitForLock(1000U);
}

/**
 * @brief Wait for PLL lock indication
 * @param timeoutUs Timeout in microseconds
 * @return STATUS_OK if locked, STATUS_TIMEOUT if not locked in time
 */
Status_t PLL_WaitForLock(uint32 timeoutUs)
{
    volatile uint32 count = timeoutUs * 100U;

    while (count > 0U)
    {
        if ((REG32_RD(PLL_STATUS) & PLL_STATUS_LOCKED) != 0U)
        {
            return STATUS_OK;
        }
        count--;
    }

    return STATUS_TIMEOUT;
}

/**
 * @brief Check PLL lock status
 */
boolean PLL_IsLocked(void)
{
    return ((REG32_RD(PLL_STATUS) & PLL_STATUS_LOCKED) != 0U) ? TRUE : FALSE;
}

/**
 * @brief Run PLL VCO calibration
 */
Status_t PLL_RunCalibration(void)
{
    if (g_pllInitialized != TRUE) return STATUS_NOT_READY;

    REG32_SET(PLL_CAL_CTRL, 0x01U);

    volatile uint32 timeout = 100000U;
    while (((REG32_RD(PLL_CAL_STATUS) & 0x01U) == 0U) && (timeout > 0U))
    {
        timeout--;
    }

    REG32_CLR(PLL_CAL_CTRL, 0x01U);

    return (timeout > 0U) ? STATUS_OK : STATUS_TIMEOUT;
}
