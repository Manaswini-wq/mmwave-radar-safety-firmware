/**
 * @file lockstep_monitor.c
 * @brief Lockstep CPU Compare Logic Monitor
 * @details Verifies that the AWR2944 R5F core is operating in lockstep
 *          mode, where two CPU cores execute the same instructions and
 *          their outputs are compared cycle-by-cycle. Any compare mismatch
 *          triggers an ESM error leading to safe-state entry.
 *
 *          Self-test capability injects a deliberate mismatch to verify
 *          the compare logic hardware is functional.
 *
 *          ISO 26262 Table 11, 1g: Lockstep comparison
 *          IEC 61508 Table A.1: Comparator (dual CPU)
 *
 * @copyright Copyright (c) 2024
 */

#include "lockstep_monitor.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"
#include "fault_handler.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static boolean g_lockstepEnabled = FALSE;
static boolean g_lockstepInitialized = FALSE;
static uint32  g_selfTestResult = 0U;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize and verify lockstep mode
 * @details Reads the R5F lockstep configuration register and verifies
 *          that lockstep mode is enabled. If not enabled, this is a
 *          configuration error that prevents ASIL-D operation.
 * @return STATUS_OK if lockstep is enabled and verified
 */
Status_t Lockstep_Init(void)
{
    uint32 lockstepReg;

    if (LOCKSTEP_ENABLE != TRUE)
    {
        /* Lockstep not required by configuration */
        g_lockstepEnabled = FALSE;
        g_lockstepInitialized = TRUE;
        return STATUS_OK;
    }

    /* Read lockstep enable register */
    lockstepReg = REG32_RD(MSS_CTRL_R5_LOCKSTEP_EN);

    if ((lockstepReg & LOCKSTEP_EN_MASK) != LOCKSTEP_EN_ENABLE)
    {
        /* Lockstep is NOT enabled -- this is a critical configuration error.
         * AWR2944 lockstep must be configured at boot via eFuse/SYSCONFIG.
         * Cannot enable lockstep from software after boot. */
        g_lockstepEnabled = FALSE;
        g_lockstepInitialized = TRUE;
        return STATUS_ERROR;
    }

    /* Verify lockstep status by reading back */
    uint32 verifyReg = REG32_RD(MSS_CTRL_R5_LOCKSTEP_EN);
    if (verifyReg != lockstepReg)
    {
        /* Read-back mismatch -- possible register corruption */
        return STATUS_ERROR;
    }

    /* Verify the lock override is not active */
    uint32 overrideReg = REG32_RD(MSS_CTRL_R5_LOCK_OVERRIDE);
    if (overrideReg != 0U)
    {
        /* Override is active -- lockstep may be bypassed */
        return STATUS_ERROR;
    }

    g_lockstepEnabled = TRUE;
    g_lockstepInitialized = TRUE;
    g_selfTestResult = 0U;

    return STATUS_OK;
}

/**
 * @brief Lockstep self-test via error injection
 * @details Triggers the lockstep self-test mechanism built into the
 *          AWR2944. This injects a deliberate mismatch between the
 *          primary and shadow CPU, which should be detected by the
 *          compare logic and reported via ESM.
 *
 *          The ESM error is expected during self-test and is handled
 *          specially (not treated as a real fault).
 *
 * @return DIAG_PASS if the injected error was detected
 */
DiagResult_t Lockstep_SelfTest(void)
{
    uint32 esmStatusBefore;
    uint32 esmStatusAfter;

    if (g_lockstepEnabled != TRUE)
    {
        return DIAG_FAIL;
    }

    /* Save current ESM Group 2 status */
    esmStatusBefore = REG32_RD(ESM_G2_STS(0));

    /* Clear lockstep compare error status */
    REG32_WR(ESM_G2_STS(0), (1U << ESM_ERR_LOCKSTEP_COMPARE));

    /* Trigger self-test by writing to the lockstep self-test register.
     * This injects a known mismatch into the compare logic. */
    REG32_WR(MSS_CTRL_R5_CONTROL, REG32_RD(MSS_CTRL_R5_CONTROL) | (1U << 8));

    /* Small delay for the compare error to propagate */
    volatile uint32 delay;
    for (delay = 0U; delay < 100U; delay++)
    {
        /* Wait for compare error to be flagged */
    }

    /* Check if ESM detected the lockstep compare error */
    esmStatusAfter = REG32_RD(ESM_G2_STS(0));

    /* De-assert self-test */
    REG32_WR(MSS_CTRL_R5_CONTROL,
             REG32_RD(MSS_CTRL_R5_CONTROL) & ~(1U << 8));

    /* Clear the self-test-induced ESM error */
    REG32_WR(ESM_G2_STS(0), (1U << ESM_ERR_LOCKSTEP_COMPARE));

    /* Verify the error was detected */
    if ((esmStatusAfter & (1U << ESM_ERR_LOCKSTEP_COMPARE)) != 0U)
    {
        g_selfTestResult = 1U;
        return DIAG_PASS;
    }
    else
    {
        /* Compare logic did not detect the injected error */
        g_selfTestResult = 0U;
        FaultHandler_Report(FAULT_LOCKSTEP_COMPARE, FAULT_SEVERITY_CRITICAL);
        return DIAG_FAIL;
    }
}

/**
 * @brief Get lockstep operational status
 * @return DIAG_PASS if lockstep is enabled and no errors detected
 */
DiagResult_t Lockstep_GetStatus(void)
{
    if (g_lockstepInitialized != TRUE)
    {
        return DIAG_FAIL;
    }

    if (LOCKSTEP_ENABLE == TRUE)
    {
        if (g_lockstepEnabled != TRUE)
        {
            return DIAG_FAIL;
        }

        /* Check for any pending lockstep compare error in ESM */
        uint32 esmStatus = REG32_RD(ESM_G2_RAW(0));
        if ((esmStatus & (1U << ESM_ERR_LOCKSTEP_COMPARE)) != 0U)
        {
            return DIAG_FAIL;
        }
    }

    return DIAG_PASS;
}

/**
 * @brief Check if lockstep is enabled
 * @return TRUE if lockstep is active
 */
boolean Lockstep_IsEnabled(void)
{
    return g_lockstepEnabled;
}
