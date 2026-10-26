/**
 * @file lo_leakage_cal.c
 * @brief LO Leakage Calibration Algorithm
 * @details Measures and nulls the LO (Local Oscillator) leakage
 *          that appears as a DC offset in the baseband I/Q signals.
 *          Uses an iterative binary-search nulling algorithm.
 *
 *          Algorithm:
 *          1. Measure DC offset on I and Q channels
 *          2. Apply compensation DAC value (binary search)
 *          3. Re-measure residual
 *          4. Repeat until residual < threshold
 *
 * @copyright Copyright (c) 2024
 */

#include "lo_leakage_cal.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static int32  g_residualI = 0;
static int32  g_residualQ = 0;
static boolean g_loCalDone = FALSE;

/* Calibration parameters */
#define LO_CAL_MAX_ITERATIONS     16U
#define LO_CAL_THRESHOLD          5     /* LSB threshold for convergence */
#define LO_CAL_DAC_RANGE          256   /* 8-bit DAC for DC offset comp */
#define LO_CAL_NUM_AVG_SAMPLES    128U

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static int32 LOCal_MeasureDCOffset(boolean iChannel);
static void LOCal_SetCompensation(int32 compI, int32 compQ);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Run LO leakage calibration
 * @return STATUS_OK if leakage is nulled below threshold
 */
Status_t LOLeakageCal_Run(void)
{
    int32 compI = 0;
    int32 compQ = 0;
    int32 stepI = LO_CAL_DAC_RANGE / 2;
    int32 stepQ = LO_CAL_DAC_RANGE / 2;
    uint32 iter;
    int32 dcI, dcQ;

    /* Enable LO leakage measurement mode */
    REG32_WR(LO_NULL_CTRL, 0x01U);

    /* Configure ADC for DC measurement (complex mode, low samples) */
    REG32_WR(ADC_DATA_FORMAT, ADC_FORMAT_COMPLEX);
    REG32_WR(ADC_NUM_SAMPLES, LO_CAL_NUM_AVG_SAMPLES);

    /* Binary search nulling on I channel */
    for (iter = 0U; iter < LO_CAL_MAX_ITERATIONS; iter++)
    {
        LOCal_SetCompensation(compI, compQ);

        dcI = LOCal_MeasureDCOffset(TRUE);

        if (dcI > 0)
        {
            compI -= stepI;
        }
        else
        {
            compI += stepI;
        }

        stepI = (stepI + 1) / 2;  /* Halve step size (round up) */

        if ((dcI > -LO_CAL_THRESHOLD) && (dcI < LO_CAL_THRESHOLD))
        {
            break;
        }
    }

    /* Binary search nulling on Q channel */
    stepQ = LO_CAL_DAC_RANGE / 2;
    for (iter = 0U; iter < LO_CAL_MAX_ITERATIONS; iter++)
    {
        LOCal_SetCompensation(compI, compQ);

        dcQ = LOCal_MeasureDCOffset(FALSE);

        if (dcQ > 0)
        {
            compQ -= stepQ;
        }
        else
        {
            compQ += stepQ;
        }

        stepQ = (stepQ + 1) / 2;

        if ((dcQ > -LO_CAL_THRESHOLD) && (dcQ < LO_CAL_THRESHOLD))
        {
            break;
        }
    }

    /* Final measurement to verify */
    LOCal_SetCompensation(compI, compQ);
    g_residualI = LOCal_MeasureDCOffset(TRUE);
    g_residualQ = LOCal_MeasureDCOffset(FALSE);

    /* Disable LO cal mode */
    REG32_WR(LO_NULL_CTRL, 0x00U);
    REG32_WR(ADC_DATA_FORMAT, ADC_FORMAT_REAL);

    /* Store final compensation values */
    REG32_WR(LO_DC_OFFSET_I, (uint32)(compI & 0xFFU));
    REG32_WR(LO_DC_OFFSET_Q, (uint32)(compQ & 0xFFU));

    /* Write residual for readback */
    REG32_WR(LO_RESIDUAL_I, (uint32)(g_residualI & 0xFFFFU));
    REG32_WR(LO_RESIDUAL_Q, (uint32)(g_residualQ & 0xFFFFU));

    g_loCalDone = TRUE;

    /* Check if residual is within spec */
    if ((g_residualI > LO_CAL_THRESHOLD) || (g_residualI < -LO_CAL_THRESHOLD) ||
        (g_residualQ > LO_CAL_THRESHOLD) || (g_residualQ < -LO_CAL_THRESHOLD))
    {
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

/**
 * @brief Get LO leakage residual after calibration
 */
Status_t LOLeakageCal_GetResidual(int32 *pResidualI, int32 *pResidualQ)
{
    if ((pResidualI == NULL_PTR) || (pResidualQ == NULL_PTR))
    {
        return STATUS_ERROR;
    }
    *pResidualI = g_residualI;
    *pResidualQ = g_residualQ;
    return STATUS_OK;
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

/**
 * @brief Measure DC offset on I or Q channel
 * @param iChannel TRUE for I channel, FALSE for Q
 * @return Average DC offset in ADC LSBs (signed)
 */
static int32 LOCal_MeasureDCOffset(boolean iChannel)
{
    int32 sum = 0;
    uint32 i;
    uint32 offset = (iChannel == TRUE) ? 0U : 4U;

    /* Trigger ADC capture */
    REG32_SET(ADC_CTRL, ADC_CTRL_ENABLE);

    volatile uint32 timeout = 50000U;
    while (((REG32_RD(ADC_INT_STATUS) & ADC_INT_BUFFER_FULL) == 0U) &&
           (timeout > 0U))
    {
        timeout--;
    }

    /* Read samples */
    for (i = 0U; i < LO_CAL_NUM_AVG_SAMPLES; i++)
    {
        uint32 addr = ADC_BUFFER_BASE + (i * 8U) + offset;
        int32 sample = (int32)(int16)(REG32_RD(addr) & 0xFFFFU);
        sum += sample;
    }

    REG32_WR(ADC_INT_CLEAR, ADC_INT_BUFFER_FULL);

    return sum / (int32)LO_CAL_NUM_AVG_SAMPLES;
}

/**
 * @brief Set DC compensation values
 */
static void LOCal_SetCompensation(int32 compI, int32 compQ)
{
    /* Clamp to DAC range */
    if (compI > (LO_CAL_DAC_RANGE / 2 - 1)) compI = LO_CAL_DAC_RANGE / 2 - 1;
    if (compI < -(LO_CAL_DAC_RANGE / 2))    compI = -(LO_CAL_DAC_RANGE / 2);
    if (compQ > (LO_CAL_DAC_RANGE / 2 - 1)) compQ = LO_CAL_DAC_RANGE / 2 - 1;
    if (compQ < -(LO_CAL_DAC_RANGE / 2))    compQ = -(LO_CAL_DAC_RANGE / 2);

    /* Write to RX DC offset correction registers */
    REG32_WR(RX_CH_DC_OFFSET_I(0), (uint32)(compI & 0xFFU));
    REG32_WR(RX_CH_DC_OFFSET_Q(0), (uint32)(compQ & 0xFFU));
}
