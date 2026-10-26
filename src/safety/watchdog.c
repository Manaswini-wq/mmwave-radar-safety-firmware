/**
 * @file watchdog.c
 * @brief Windowed Watchdog Timer Implementation
 * @details Implements a windowed watchdog timer using the AWR2944 RTI DWD
 *          (Digital Windowed Watchdog). The window mechanism ensures the
 *          firmware services the watchdog within a valid time window,
 *          detecting both too-early and too-late servicing.
 *
 *          Any window violation triggers an NMI via ESM, leading to
 *          fail-safe reset.
 *
 *          ISO 26262 Table 11, 3a: Watchdog with time window
 *          IEC 61508 Table A.6: Temporal and logical program monitoring
 *
 * @copyright Copyright (c) 2024
 */

#include "watchdog.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static boolean g_wdtInitialized = FALSE;
static boolean g_wdtEnabled     = FALSE;
static uint32  g_wdtPreloadVal  = 0U;
static uint32  g_wdtWindowSize  = 0U;

/** @brief Service sequence state (two-key sequence required) */
static uint32 g_serviceState = 0U;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize the windowed watchdog timer
 * @param timeoutMs Watchdog timeout period in milliseconds
 * @param windowMinMs Minimum time before service is accepted
 * @return STATUS_OK on success
 */
Status_t Watchdog_Init(uint32 timeoutMs, uint32 windowMinMs)
{
    /* Calculate preload value based on RTI clock (200 MHz typical) */
    /* Preload = (timeout_ms * RTI_CLK_MHZ * 1000) / (2^13) */
    uint32 rtiClkMHz = 200U;
    g_wdtPreloadVal = (timeoutMs * rtiClkMHz * 1000U) >> 13U;

    /* Determine window size (as fraction of timeout) */
    if (windowMinMs == 0U)
    {
        g_wdtWindowSize = WDT_WINDOW_100_PERCENT;  /* No window restriction */
    }
    else if (windowMinMs <= (timeoutMs / 4U))
    {
        g_wdtWindowSize = WDT_WINDOW_25_PERCENT;
    }
    else
    {
        g_wdtWindowSize = WDT_WINDOW_50_PERCENT;
    }

    /* Set preload value */
    REG32_WR(WDT_RTIDWDPRLD, g_wdtPreloadVal & 0x00000FFFU);

    /* Configure window size */
    REG32_WR(WDT_RTIWWDSIZECTRL, g_wdtWindowSize);

    /* Configure reaction (reset on violation) */
    REG32_WR(WDT_RTIWWDRXNCTRL, 0x00000005U);  /* Generate reset */

    /* Clear any pending status */
    REG32_WR(WDT_RTIWDSTATUS, 0x0000FFFFU);

    g_wdtInitialized = TRUE;
    g_serviceState = 0U;

    return STATUS_OK;
}

/**
 * @brief Enable the watchdog timer
 * @return STATUS_OK on success
 */
Status_t Watchdog_Enable(void)
{
    if (g_wdtInitialized != TRUE)
    {
        return STATUS_ERROR;
    }

    /* Enable DWD (once enabled, cannot be disabled until reset) */
    REG32_WR(WDT_RTIDWDCTRL, WDT_CTRL_ENABLE);

    g_wdtEnabled = TRUE;

    return STATUS_OK;
}

/**
 * @brief Disable the watchdog (only for debug/test builds)
 * @return STATUS_OK or STATUS_ERROR if WDT cannot be disabled
 */
Status_t Watchdog_Disable(void)
{
    /* On AWR2944, the DWD cannot be disabled once enabled.
     * This function is only effective if called before Enable. */
    if (g_wdtEnabled == TRUE)
    {
        return STATUS_ERROR;  /* Cannot disable after enable */
    }

    g_wdtInitialized = FALSE;
    return STATUS_OK;
}

/**
 * @brief Service (kick) the watchdog timer
 * @details Must be called within the valid service window.
 *          Uses a two-key sequence to prevent accidental servicing.
 *
 *          Calling too early (before window opens) or too late
 *          (after timeout) triggers a violation.
 */
void Watchdog_Service(void)
{
    if (g_wdtEnabled != TRUE)
    {
        return;
    }

    /* Two-key service sequence per AWR2944 TRM */
    if (g_serviceState == 0U)
    {
        REG32_WR(WDT_RTIWDKEY, WDT_KEY_FIRST);
        g_serviceState = 1U;
    }

    if (g_serviceState == 1U)
    {
        REG32_WR(WDT_RTIWDKEY, WDT_KEY_SECOND);
        g_serviceState = 0U;
    }
}

/**
 * @brief Check if watchdog is actively counting
 * @return TRUE if enabled and running
 */
boolean Watchdog_IsRunning(void)
{
    if (g_wdtEnabled != TRUE)
    {
        return FALSE;
    }

    /* Read down-counter; if non-zero, WDT is running */
    uint32 counter = REG32_RD(WDT_RTIDWDCNTR);
    return (counter != 0U) ? TRUE : FALSE;
}

/**
 * @brief Check for watchdog window violation
 * @return TRUE if a violation has been detected
 */
boolean Watchdog_CheckViolation(void)
{
    uint32 status = REG32_RD(WDT_RTIWDSTATUS);

    if ((status & WDT_STATUS_VIOLATION) != 0U)
    {
        return TRUE;
    }

    if ((status & WDT_STATUS_KEY_SEQ_ERR) != 0U)
    {
        return TRUE;
    }

    if ((status & WDT_STATUS_END_TIME_VIOL) != 0U)
    {
        return TRUE;
    }

    return FALSE;
}
