/**
 * @file chirp_config.c
 * @brief Chirp Configuration Implementation
 * @details Programs the AWR2944 chirp engine with FMCW chirp parameters:
 *          start frequency, frequency slope, idle time, ADC sampling,
 *          and multi-chirp sequences with TX/RX channel enable masks.
 *
 *          Frequency word calculation:
 *          freq_word = (freq_GHz * 2^26) / f_SYNTH_CLK
 *          where f_SYNTH_CLK = 40 GHz for 77 GHz band
 *
 * @copyright Copyright (c) 2024
 */

#include "chirp_config.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static ChirpProfile_t g_profiles[RADAR_MAX_CHIRP_PROFILES];
static FrameConfig_t  g_frameConfig;
static uint32 g_profileCount = 0U;

/* ========================================================================== */
/*                       Frequency Conversion Constants                       */
/* ========================================================================== */

#define SYNTH_CLK_GHZ       40.0f
#define FREQ_WORD_SCALE     67108864.0f  /* 2^26 */
#define SLOPE_WORD_SCALE    8388608.0f   /* 2^23 */
#define TIME_UNIT_NS        10U          /* 10 ns resolution */

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static uint32 ChirpConfig_FreqToWord(float32 freqGHz);
static uint32 ChirpConfig_SlopeToWord(float32 slopeMHzUs);
static uint32 ChirpConfig_TimeToCount(float32 timeUs);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Configure a chirp profile
 * @param pProfile Pointer to chirp profile parameters
 * @return STATUS_OK on success
 */
Status_t ChirpConfig_SetProfile(const ChirpProfile_t *pProfile)
{
    uint32 freqWordL, freqWordH;
    uint32 slopeWordL, slopeWordH;
    uint32 idleCount, adcStartCount;
    uint32 freqWord, slopeWord;

    if (pProfile == NULL_PTR)
    {
        return STATUS_ERROR;
    }

    if (pProfile->profileId >= RADAR_MAX_CHIRP_PROFILES)
    {
        return STATUS_ERROR;
    }

    if (pProfile->adcSamples > RADAR_MAX_ADC_SAMPLES)
    {
        return STATUS_ERROR;
    }

    /* Store profile locally */
    g_profiles[pProfile->profileId] = *pProfile;

    /* Convert frequency to hardware word */
    freqWord = ChirpConfig_FreqToWord(pProfile->startFreqGHz);
    freqWordL = freqWord & 0xFFFFU;
    freqWordH = (freqWord >> 16U) & 0xFFFFU;

    /* Convert slope to hardware word */
    slopeWord = ChirpConfig_SlopeToWord(pProfile->freqSlopeMHzUs);
    slopeWordL = slopeWord & 0xFFFFU;
    slopeWordH = (slopeWord >> 16U) & 0xFFFFU;

    /* Convert timing parameters */
    idleCount = ChirpConfig_TimeToCount(pProfile->idleTimeUs);
    adcStartCount = ChirpConfig_TimeToCount(pProfile->adcStartTimeUs);

    /* Select profile index */
    REG32_WR(CHIRP_PROFILE_IDX, pProfile->profileId);

    /* Program chirp parameters */
    REG32_WR(CHIRP_START_FREQ_L, freqWordL);
    REG32_WR(CHIRP_START_FREQ_H, freqWordH);
    REG32_WR(CHIRP_FREQ_SLOPE_L, slopeWordL);
    REG32_WR(CHIRP_FREQ_SLOPE_H, slopeWordH);
    REG32_WR(CHIRP_IDLE_TIME, idleCount);
    REG32_WR(CHIRP_ADC_START_TIME, adcStartCount);
    REG32_WR(CHIRP_NUM_ADC_SAMPLES, pProfile->adcSamples);

    /* Set TX/RX channel enable masks */
    REG32_WR(CHIRP_TX_EN_MASK, pProfile->txChannelMask & 0x0FU);
    REG32_WR(CHIRP_RX_EN_MASK, pProfile->rxChannelMask & 0x0FU);

    /* Configure ADC sample rate */
    REG32_WR(ADC_SAMPLE_RATE, pProfile->sampleRateMsps);

    if (pProfile->profileId >= g_profileCount)
    {
        g_profileCount = pProfile->profileId + 1U;
    }

    return STATUS_OK;
}

/**
 * @brief Configure frame parameters
 * @param pFrame Pointer to frame configuration
 * @return STATUS_OK on success
 */
Status_t ChirpConfig_SetFrame(const FrameConfig_t *pFrame)
{
    if (pFrame == NULL_PTR)
    {
        return STATUS_ERROR;
    }

    g_frameConfig = *pFrame;

    /* Program frame parameters to hardware */
    REG32_WR(CHIRP_NUM_LOOPS, pFrame->numChirpsPerFrame);
    REG32_WR(CHIRP_NUM_FRAMES, pFrame->numFrames);

    /* Convert frame period from ms to hardware count */
    /* Frame period register: count in units of 5ns at 200 MHz */
    uint32 framePeriodCount = (uint32)(pFrame->framePeriodMs * 200000.0f);
    REG32_WR(CHIRP_FRAME_PERIOD, framePeriodCount);

    /* Set trigger mode */
    if (pFrame->triggerMode == FRAME_TRIGGER_SW)
    {
        REG32_SET(CHIRP_CTRL, CHIRP_CTRL_SW_TRIGGER);
    }
    else
    {
        REG32_CLR(CHIRP_CTRL, CHIRP_CTRL_SW_TRIGGER);
    }

    return STATUS_OK;
}

/**
 * @brief Retrieve a stored chirp profile
 */
Status_t ChirpConfig_GetProfile(uint32 profileId, ChirpProfile_t *pProfile)
{
    if ((pProfile == NULL_PTR) || (profileId >= RADAR_MAX_CHIRP_PROFILES))
    {
        return STATUS_ERROR;
    }

    *pProfile = g_profiles[profileId];
    return STATUS_OK;
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

static uint32 ChirpConfig_FreqToWord(float32 freqGHz)
{
    return (uint32)((freqGHz / SYNTH_CLK_GHZ) * FREQ_WORD_SCALE);
}

static uint32 ChirpConfig_SlopeToWord(float32 slopeMHzUs)
{
    /* Convert MHz/us to GHz/s, then to hardware word */
    float32 slopeGHzS = slopeMHzUs * 1000.0f;
    return (uint32)((slopeGHzS / (SYNTH_CLK_GHZ * 1e9f)) * SLOPE_WORD_SCALE);
}

static uint32 ChirpConfig_TimeToCount(float32 timeUs)
{
    /* Convert microseconds to 10ns counts */
    return (uint32)(timeUs * 100.0f);
}
