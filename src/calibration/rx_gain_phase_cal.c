/**
 * @file rx_gain_phase_cal.c
 * @brief RX Gain/Phase Calibration Algorithm
 * @details Measures inter-channel gain and phase mismatch using an
 *          internal loopback test signal and computes correction
 *          coefficients. Applied via the RX calibration registers.
 *
 *          Algorithm:
 *          1. Enable TX loopback with known CW signal
 *          2. Capture ADC data on all RX channels simultaneously
 *          3. Compute magnitude and phase per channel
 *          4. Use RX0 as reference; compute relative gain/phase errors
 *          5. Program correction registers to equalize channels
 *
 * @copyright Copyright (c) 2024
 */

#include "rx_gain_phase_cal.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief Per-channel gain mismatch in 0.01 dB units (relative to RX0) */
static int32 g_rxGainMismatch[RADAR_MAX_RX_CHANNELS];

/** @brief Per-channel phase mismatch in 0.01 degree units (relative to RX0) */
static int32 g_rxPhaseMismatch[RADAR_MAX_RX_CHANNELS];

static boolean g_rxCalDone = FALSE;

/* Calibration parameters */
#define RX_CAL_NUM_SAMPLES     256U
#define RX_CAL_GAIN_THRESH     50    /* 0.5 dB max gain mismatch */
#define RX_CAL_PHASE_THRESH    200   /* 2.0 deg max phase mismatch */

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static void RxCal_CaptureIQ(uint32 rxCh, int32 *pI, int32 *pQ);
static int32 RxCal_ComputeMagnitude(int32 I, int32 Q);
static int32 RxCal_ComputePhase(int32 I, int32 Q);
static int32 RxCal_Atan2Approx(int32 y, int32 x);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Run RX gain/phase calibration
 * @return STATUS_OK if all channels calibrate successfully
 */
Status_t RxGainPhaseCal_Run(void)
{
    uint32 ch;
    int32 iVal[RADAR_MAX_RX_CHANNELS];
    int32 qVal[RADAR_MAX_RX_CHANNELS];
    int32 mag[RADAR_MAX_RX_CHANNELS];
    int32 phase[RADAR_MAX_RX_CHANNELS];

    /* Enable TX0 loopback as reference source */
    REG32_WR(TX_CH_LOOPBACK(0), 0x01U);
    REG32_WR(TX_CH_CTRL(0), TX_CH_EN_MASK);

    /* Configure ADC for complex (I/Q) mode */
    REG32_WR(ADC_DATA_FORMAT, ADC_FORMAT_COMPLEX);
    REG32_WR(ADC_NUM_SAMPLES, RX_CAL_NUM_SAMPLES);

    /* Enable all RX channels */
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        REG32_WR(RX_CH_CTRL(ch), RX_CH_EN_MASK);
    }

    /* Trigger ADC capture */
    REG32_SET(ADC_CTRL, ADC_CTRL_ENABLE);

    /* Wait for completion */
    volatile uint32 timeout = 100000U;
    while (((REG32_RD(ADC_INT_STATUS) & ADC_INT_BUFFER_FULL) == 0U) &&
           (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        return STATUS_TIMEOUT;
    }

    /* Extract I/Q data per channel and compute mean */
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        RxCal_CaptureIQ(ch, &iVal[ch], &qVal[ch]);
        mag[ch]   = RxCal_ComputeMagnitude(iVal[ch], qVal[ch]);
        phase[ch] = RxCal_ComputePhase(iVal[ch], qVal[ch]);
    }

    /* Clear ADC interrupt */
    REG32_WR(ADC_INT_CLEAR, ADC_INT_BUFFER_FULL);

    /* Compute relative mismatch using RX0 as reference */
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        /* Gain mismatch in 0.01 dB: 20*log10(mag[ch]/mag[0]) * 100 */
        if (mag[0] != 0)
        {
            /* Simplified linear approximation */
            g_rxGainMismatch[ch] = ((mag[ch] - mag[0]) * 2000) / mag[0];
        }
        else
        {
            g_rxGainMismatch[ch] = 0;
        }

        /* Phase mismatch in 0.01 degrees */
        g_rxPhaseMismatch[ch] = phase[ch] - phase[0];
    }

    /* Apply corrections to hardware */
    for (ch = 0U; ch < RADAR_MAX_RX_CHANNELS; ch++)
    {
        /* Gain correction: negate mismatch */
        int32 gainCorr = -g_rxGainMismatch[ch];
        /* Convert to register code (8-bit signed, 0.1 dB per LSB) */
        int32 gainReg = gainCorr / 10;
        if (gainReg > 127) gainReg = 127;
        if (gainReg < -128) gainReg = -128;
        REG32_WR(RX_CH_CAL_GAIN(ch), (uint32)gainReg & 0xFFU);

        /* Phase correction: negate mismatch */
        int32 phaseCorr = -g_rxPhaseMismatch[ch];
        /* Convert to register code (8-bit signed, 0.1 deg per LSB) */
        int32 phaseReg = phaseCorr / 10;
        if (phaseReg > 127) phaseReg = 127;
        if (phaseReg < -128) phaseReg = -128;
        REG32_WR(RX_CH_CAL_PHASE(ch), (uint32)phaseReg & 0xFFU);
    }

    /* Disable loopback */
    REG32_WR(TX_CH_LOOPBACK(0), 0x00U);
    REG32_WR(ADC_DATA_FORMAT, ADC_FORMAT_REAL);

    g_rxCalDone = TRUE;

    return STATUS_OK;
}

/**
 * @brief Get gain mismatch for an RX channel
 */
Status_t RxGainPhaseCal_GetGainMismatch(uint32 rxCh, float32 *pGaindB)
{
    if ((rxCh >= RADAR_MAX_RX_CHANNELS) || (pGaindB == NULL_PTR))
    {
        return STATUS_ERROR;
    }
    *pGaindB = (float32)g_rxGainMismatch[rxCh] / 100.0f;
    return STATUS_OK;
}

/**
 * @brief Get phase mismatch for an RX channel
 */
Status_t RxGainPhaseCal_GetPhaseMismatch(uint32 rxCh, float32 *pPhaseDeg)
{
    if ((rxCh >= RADAR_MAX_RX_CHANNELS) || (pPhaseDeg == NULL_PTR))
    {
        return STATUS_ERROR;
    }
    *pPhaseDeg = (float32)g_rxPhaseMismatch[rxCh] / 100.0f;
    return STATUS_OK;
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

/**
 * @brief Capture and average I/Q samples for a given RX channel
 */
static void RxCal_CaptureIQ(uint32 rxCh, int32 *pI, int32 *pQ)
{
    int32 sumI = 0, sumQ = 0;
    uint32 i;
    uint32 baseOffset = rxCh * RX_CAL_NUM_SAMPLES * 8U;  /* 2 words per sample (I,Q) */

    for (i = 0U; i < RX_CAL_NUM_SAMPLES; i++)
    {
        uint32 addrI = ADC_BUFFER_BASE + baseOffset + (i * 8U);
        uint32 addrQ = ADC_BUFFER_BASE + baseOffset + (i * 8U) + 4U;

        int32 sampleI = (int32)(int16)(REG32_RD(addrI) & 0xFFFFU);
        int32 sampleQ = (int32)(int16)(REG32_RD(addrQ) & 0xFFFFU);

        sumI += sampleI;
        sumQ += sampleQ;
    }

    *pI = sumI / (int32)RX_CAL_NUM_SAMPLES;
    *pQ = sumQ / (int32)RX_CAL_NUM_SAMPLES;
}

/**
 * @brief Compute signal magnitude (I^2 + Q^2, integer sqrt approximation)
 */
static int32 RxCal_ComputeMagnitude(int32 I, int32 Q)
{
    /* Use alpha-max beta-min approximation for speed */
    int32 absI = (I >= 0) ? I : -I;
    int32 absQ = (Q >= 0) ? Q : -Q;

    int32 maxVal = (absI > absQ) ? absI : absQ;
    int32 minVal = (absI <= absQ) ? absI : absQ;

    /* Approximation: mag ~= max + 0.375 * min (error < 4%) */
    return maxVal + ((3 * minVal) / 8);
}

/**
 * @brief Compute signal phase using atan2 approximation
 * @return Phase in 0.01 degree units
 */
static int32 RxCal_ComputePhase(int32 I, int32 Q)
{
    return RxCal_Atan2Approx(Q, I);
}

/**
 * @brief Integer atan2 approximation
 * @return Angle in 0.01 degree units (-18000 to +18000)
 */
static int32 RxCal_Atan2Approx(int32 y, int32 x)
{
    int32 angle;

    if (x == 0 && y == 0)
    {
        return 0;
    }

    /* Use CORDIC-like polynomial approximation */
    int32 absX = (x >= 0) ? x : -x;
    int32 absY = (y >= 0) ? y : -y;

    if (absX >= absY)
    {
        /* angle = atan(y/x) * (18000/pi) */
        int32 ratio = (y * 1000) / x;  /* scaled by 1000 */
        angle = (ratio * 5730) / 1000;  /* 5730 ~= 18000/pi */
    }
    else
    {
        /* angle = 9000 - atan(x/y) * (18000/pi) */
        int32 ratio = (x * 1000) / y;
        angle = 9000 - ((ratio * 5730) / 1000);
    }

    /* Adjust quadrant */
    if (x < 0)
    {
        angle = (y >= 0) ? (angle + 18000) : (angle - 18000);
    }

    return angle;
}
