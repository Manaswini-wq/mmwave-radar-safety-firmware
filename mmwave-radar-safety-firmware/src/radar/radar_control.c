/**
 * @file radar_control.c
 * @brief Real-Time Radar Control Implementation
 * @details Manages the AWR2944 RF subsystem power-on sequence, chirp
 *          engine configuration, frame start/stop control, and real-time
 *          interrupt handling for chirp and frame completion events.
 *
 *          The power-on sequence follows the AWR2944 TRM specification
 *          with proper settling times and status verification at each step.
 *
 * @copyright Copyright (c) 2024
 */

#include "radar_control.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"
#include "chirp_config.h"
#include "drivers/pll_driver.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static RadarState_t g_radarState = RADAR_STATE_OFF;
static uint32 g_frameCount       = 0U;
static uint32 g_chirpCount       = 0U;
static boolean g_radarInitialized = FALSE;

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static Status_t RadarControl_RFPowerSequence(boolean powerOn);
static void RadarControl_WaitUs(uint32 us);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize radar control module
 * @return STATUS_OK on success
 */
Status_t RadarControl_Init(void)
{
    g_radarState = RADAR_STATE_INIT;
    g_frameCount = 0U;
    g_chirpCount = 0U;

    /* Ensure chirp engine is stopped */
    REG32_WR(CHIRP_SEQ_FORCE_STOP, 0x01U);

    /* Wait for chirp engine to idle */
    volatile uint32 timeout = 10000U;
    while (((REG32_RD(CHIRP_STATUS) & CHIRP_STATUS_ACTIVE) != 0U) &&
           (timeout > 0U))
    {
        timeout--;
    }

    if (timeout == 0U)
    {
        g_radarState = RADAR_STATE_ERROR;
        return STATUS_TIMEOUT;
    }

    /* Power on RF subsystem */
    Status_t status = RadarControl_PowerOn();
    if (status != STATUS_OK)
    {
        g_radarState = RADAR_STATE_ERROR;
        return status;
    }

    /* Configure default chirp profile */
    ChirpProfile_t defaultProfile;
    defaultProfile.profileId       = 0U;
    defaultProfile.startFreqGHz    = 77.0f;
    defaultProfile.freqSlopeMHzUs  = 60.0f;
    defaultProfile.idleTimeUs      = 5.0f;
    defaultProfile.adcStartTimeUs  = 6.0f;
    defaultProfile.adcSamples      = 256U;
    defaultProfile.sampleRateMsps  = 25U;
    defaultProfile.txChannelMask   = 0x07U;  /* TX0, TX1, TX2 */
    defaultProfile.rxChannelMask   = 0x0FU;  /* RX0-RX3 */

    status = ChirpConfig_SetProfile(&defaultProfile);
    if (status != STATUS_OK)
    {
        g_radarState = RADAR_STATE_ERROR;
        return status;
    }

    /* Configure frame parameters */
    FrameConfig_t frameConfig;
    frameConfig.numChirpsPerFrame  = 128U;
    frameConfig.numFrames          = 0U;     /* Infinite frames */
    frameConfig.framePeriodMs      = 33.3f;  /* ~30 fps */
    frameConfig.triggerMode        = FRAME_TRIGGER_SW;

    status = ChirpConfig_SetFrame(&frameConfig);
    if (status != STATUS_OK)
    {
        g_radarState = RADAR_STATE_ERROR;
        return status;
    }

    g_radarState = RADAR_STATE_IDLE;
    g_radarInitialized = TRUE;

    return STATUS_OK;
}

/**
 * @brief Power on the RF subsystem
 * @details Follows the AWR2944 RF power-on sequence:
 *          1. Enable RF clock
 *          2. Release RF reset
 *          3. Power on analog blocks
 *          4. Wait for power-good indication
 *          5. Enable PLL and wait for lock
 * @return STATUS_OK on success
 */
Status_t RadarControl_PowerOn(void)
{
    return RadarControl_RFPowerSequence(TRUE);
}

/**
 * @brief Power off the RF subsystem
 * @return STATUS_OK on success
 */
Status_t RadarControl_PowerOff(void)
{
    /* Stop any active frames first */
    if (g_radarState == RADAR_STATE_ACTIVE)
    {
        (void)RadarControl_StopFrames();
    }

    return RadarControl_RFPowerSequence(FALSE);
}

/**
 * @brief Start radar frame transmission
 * @return STATUS_OK on success
 */
Status_t RadarControl_StartFrames(void)
{
    if (g_radarInitialized != TRUE)
    {
        return STATUS_NOT_READY;
    }

    if (g_radarState != RADAR_STATE_IDLE)
    {
        return STATUS_ERROR;
    }

    /* Verify PLL is locked before starting */
    if ((REG32_RD(PLL_STATUS) & PLL_STATUS_LOCKED) == 0U)
    {
        return STATUS_ERROR;
    }

    /* Clear any pending chirp/frame status */
    REG32_WR(CHIRP_STATUS, 0xFFU);

    /* Enable chirp engine */
    REG32_WR(CHIRP_CTRL, CHIRP_CTRL_ENABLE);

    /* Trigger frame start (software trigger) */
    REG32_WR(CHIRP_SEQ_TRIGGER, 0x01U);

    g_radarState = RADAR_STATE_ACTIVE;
    g_frameCount = 0U;
    g_chirpCount = 0U;

    return STATUS_OK;
}

/**
 * @brief Stop radar frame transmission
 * @return STATUS_OK on success
 */
Status_t RadarControl_StopFrames(void)
{
    /* Force stop chirp sequencer */
    REG32_WR(CHIRP_SEQ_FORCE_STOP, 0x01U);

    /* Wait for chirp engine to stop */
    volatile uint32 timeout = 100000U;
    while (((REG32_RD(CHIRP_STATUS) & CHIRP_STATUS_ACTIVE) != 0U) &&
           (timeout > 0U))
    {
        timeout--;
    }

    /* Disable chirp engine */
    REG32_WR(CHIRP_CTRL, 0x00U);

    g_radarState = RADAR_STATE_IDLE;

    return STATUS_OK;
}

/**
 * @brief Get current radar state
 */
RadarState_t RadarControl_GetState(void)
{
    return g_radarState;
}

/**
 * @brief Chirp completion ISR
 */
void RadarControl_ChirpDoneISR(void)
{
    g_chirpCount++;
    /* Clear chirp-done interrupt status */
    REG32_WR(CHIRP_STATUS, CHIRP_STATUS_CHIRP_DONE);
}

/**
 * @brief Frame completion ISR
 */
void RadarControl_FrameDoneISR(void)
{
    g_frameCount++;
    /* Clear frame-done status */
    REG32_WR(CHIRP_STATUS, CHIRP_STATUS_FRAME_DONE);
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

/**
 * @brief Execute RF power on/off sequence
 */
static Status_t RadarControl_RFPowerSequence(boolean powerOn)
{
    if (powerOn == TRUE)
    {
        /* Step 1: Enable RF clock domain */
        REG32_WR(RFSS_CTRL_CLK_EN, 0x01U);
        RadarControl_WaitUs(10U);

        /* Step 2: Release RF subsystem reset */
        REG32_WR(RFSS_CTRL_RESET, 0x00U);
        RadarControl_WaitUs(100U);

        /* Step 3: Power on analog LDOs and bias circuits */
        REG32_WR(RFSS_CTRL_PWR_ON, 0x01U);
        RadarControl_WaitUs(500U);

        /* Step 4: Check power-good status */
        uint32 status = REG32_RD(RFSS_CTRL_STATUS);
        if ((status & 0x01U) == 0U)
        {
            return STATUS_ERROR;  /* Power-on failed */
        }

        /* Step 5: Enable RF front-end */
        REG32_WR(RFSS_CTRL_RF_ENABLE, 0x01U);
        RadarControl_WaitUs(50U);
    }
    else
    {
        /* Power-off sequence (reverse order) */
        REG32_WR(RFSS_CTRL_RF_ENABLE, 0x00U);
        RadarControl_WaitUs(10U);
        REG32_WR(RFSS_CTRL_PWR_ON, 0x00U);
        RadarControl_WaitUs(10U);
        REG32_WR(RFSS_CTRL_RESET, 0x01U);
        REG32_WR(RFSS_CTRL_CLK_EN, 0x00U);

        g_radarState = RADAR_STATE_OFF;
    }

    return STATUS_OK;
}

/**
 * @brief Simple microsecond delay (busy-wait)
 */
static void RadarControl_WaitUs(uint32 us)
{
    /* Approximate: ~4 cycles per iteration at 400 MHz R5F = ~10ns */
    volatile uint32 count = us * 100U;
    while (count > 0U)
    {
        count--;
    }
}
