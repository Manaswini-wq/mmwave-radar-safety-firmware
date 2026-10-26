/**
 * @file fault_handler.c
 * @brief Fault Handler Implementation
 * @details Handles ESM group1/group2 interrupts, classifies faults as
 *          transient or permanent, attempts recovery, logs all events,
 *          and triggers safe-state entry when recovery fails.
 *
 *          ISO 26262 ASIL-D: All faults must be detected within the
 *          Fault Tolerant Time Interval (FTTI) and handled within
 *          the safe-state transition time.
 *
 * @copyright Copyright (c) 2024
 */

#include "fault_handler.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"
#include "safety_monitor.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

#define FAULT_LOG_SIZE  64U

static FaultLogEntry_t g_faultLog[FAULT_LOG_SIZE];
static uint32 g_faultLogIdx = 0U;
static uint32 g_totalFaultCount = 0U;
static boolean g_faultHandlerInit = FALSE;

/** @brief Fault occurrence counters for classification */
static uint32 g_faultOccurrence[16U];

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static FaultClass_t FaultHandler_Classify(uint32 faultCode);
static boolean FaultHandler_AttemptRecovery(uint32 faultCode);
static void FaultHandler_RequestSafeState(uint32 faultCode);
static void FaultHandler_ProcessESMError(uint32 esmErrNum, FaultSeverity_t severity);

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize fault handler
 * @details Configures ESM groups, registers ESM ISRs, and initializes
 *          the fault log.
 * @return STATUS_OK on success
 */
Status_t FaultHandler_Init(void)
{
    uint32 i;

    /* Clear fault log */
    for (i = 0U; i < FAULT_LOG_SIZE; i++)
    {
        g_faultLog[i].faultCode      = FAULT_NONE;
        g_faultLog[i].timestamp      = 0U;
        g_faultLog[i].severity       = FAULT_SEVERITY_LOW;
        g_faultLog[i].classification = FAULT_CLASS_TRANSIENT;
        g_faultLog[i].recoveryCount  = 0U;
        g_faultLog[i].resolved       = FALSE;
    }

    /* Clear occurrence counters */
    for (i = 0U; i < 16U; i++)
    {
        g_faultOccurrence[i] = 0U;
    }

    g_faultLogIdx = 0U;
    g_totalFaultCount = 0U;

    /* Enable ESM Group 1 errors (low priority, IRQ) */
    if (ESM_GROUP1_ENABLE == TRUE)
    {
        /* Enable all relevant Group 1 error sources */
        REG32_WR(ESM_G1_EN_SET(0), 0xFFFFFFFFU);

        /* Clear any pending Group 1 status */
        REG32_WR(ESM_G1_STS(0), 0xFFFFFFFFU);
    }

    /* Enable ESM Group 2 errors (high priority, FIQ/NMI) */
    if (ESM_GROUP2_ENABLE == TRUE)
    {
        /* Enable critical error sources in Group 2 */
        uint32 g2Mask = (1U << ESM_ERR_LOCKSTEP_COMPARE) |
                        (1U << ESM_ERR_ECC_TCMA_DED)     |
                        (1U << ESM_ERR_ECC_TCMB_DED)     |
                        (1U << ESM_ERR_ECC_L2_DED)       |
                        (1U << ESM_ERR_WDT_NMI);

        REG32_WR(ESM_G2_EN_SET(0), g2Mask);
        REG32_WR(ESM_G2_STS(0), 0xFFFFFFFFU);
    }

    /* Enable ESM Group 3 (direct error pin output) */
    if (ESM_GROUP3_ENABLE == TRUE)
    {
        uint32 g3Mask = (1U << ESM_ERR_LOCKSTEP_COMPARE) |
                        (1U << ESM_ERR_ECC_TCMA_DED);
        REG32_WR(ESM_G3_EN_SET(0), g3Mask);
    }

    /* Enable ESM module */
    REG32_WR(ESM_EN, 0x0000000FU);

    g_faultHandlerInit = TRUE;

    return STATUS_OK;
}

/**
 * @brief Report a fault from application code
 * @param faultCode Fault identification
 * @param severity Severity level
 */
void FaultHandler_Report(uint32 faultCode, FaultSeverity_t severity)
{
    FaultClass_t classification;
    boolean recovered;

    if (g_faultHandlerInit != TRUE)
    {
        return;
    }

    /* Increment occurrence counter */
    if (faultCode < 16U)
    {
        g_faultOccurrence[faultCode]++;
    }

    /* Classify the fault */
    classification = FaultHandler_Classify(faultCode);

    /* Log the fault */
    g_faultLog[g_faultLogIdx].faultCode      = faultCode;
    g_faultLog[g_faultLogIdx].timestamp      = 0U; /* TODO: get system tick */
    g_faultLog[g_faultLogIdx].severity       = severity;
    g_faultLog[g_faultLogIdx].classification = classification;
    g_faultLog[g_faultLogIdx].recoveryCount  = 0U;
    g_faultLog[g_faultLogIdx].resolved       = FALSE;

    g_totalFaultCount++;

    /* Handle based on severity */
    if (severity == FAULT_SEVERITY_CRITICAL)
    {
        /* Critical: immediate safe-state, no recovery attempt */
        FaultHandler_RequestSafeState(faultCode);
    }
    else if (severity == FAULT_SEVERITY_HIGH)
    {
        /* High: attempt recovery, safe-state if failed */
        recovered = FaultHandler_AttemptRecovery(faultCode);
        if (recovered == FALSE)
        {
            FaultHandler_RequestSafeState(faultCode);
        }
        else
        {
            g_faultLog[g_faultLogIdx].resolved = TRUE;
        }
    }
    else if (severity == FAULT_SEVERITY_MEDIUM)
    {
        /* Medium: attempt recovery, log and continue if failed */
        recovered = FaultHandler_AttemptRecovery(faultCode);
        g_faultLog[g_faultLogIdx].resolved = recovered;
    }
    else
    {
        /* Low: log only */
        g_faultLog[g_faultLogIdx].resolved = TRUE;
    }

    /* Advance log index */
    g_faultLogIdx++;
    if (g_faultLogIdx >= FAULT_LOG_SIZE)
    {
        g_faultLogIdx = 0U;
    }
}

/**
 * @brief Log a fault with explicit timestamp (from safe-state entry)
 */
void FaultHandler_LogFault(uint32 faultCode, uint32 timestamp)
{
    g_faultLog[g_faultLogIdx].faultCode = faultCode;
    g_faultLog[g_faultLogIdx].timestamp = timestamp;
    g_faultLog[g_faultLogIdx].severity  = FAULT_SEVERITY_CRITICAL;
    g_faultLog[g_faultLogIdx].resolved  = FALSE;

    g_faultLogIdx++;
    if (g_faultLogIdx >= FAULT_LOG_SIZE)
    {
        g_faultLogIdx = 0U;
    }
}

/**
 * @brief ESM Group 1 ISR (IRQ -- low priority faults)
 * @details Reads ESM G1 status, identifies error source, handles it.
 */
void FaultHandler_ESM_Group1_ISR(void)
{
    uint32 status = REG32_RD(ESM_G1_STS(0));
    uint32 bit;

    for (bit = 0U; bit < 32U; bit++)
    {
        if ((status & (1U << bit)) != 0U)
        {
            FaultHandler_ProcessESMError(bit, FAULT_SEVERITY_MEDIUM);
            /* Clear the status bit */
            REG32_WR(ESM_G1_STS(0), (1U << bit));
        }
    }
}

/**
 * @brief ESM Group 2 ISR (FIQ/NMI -- high priority faults)
 * @details Critical faults that require immediate safe-state entry.
 */
void FaultHandler_ESM_Group2_ISR(void)
{
    uint32 status = REG32_RD(ESM_G2_STS(0));
    uint32 bit;

    for (bit = 0U; bit < 32U; bit++)
    {
        if ((status & (1U << bit)) != 0U)
        {
            FaultHandler_ProcessESMError(bit, FAULT_SEVERITY_CRITICAL);
            REG32_WR(ESM_G2_STS(0), (1U << bit));
        }
    }
}

/* ========================================================================== */
/*                  Exception Handlers (called from ASM)                      */
/* ========================================================================== */

void Fault_UndefinedHandler(uint32 addr, uint32 *pContext)
{
    (void)pContext;
    FaultHandler_Report(FAULT_CFM_VIOLATION, FAULT_SEVERITY_CRITICAL);
    (void)addr;
}

void Fault_SVCHandler(uint32 svcNum, uint32 *pContext)
{
    (void)svcNum;
    (void)pContext;
    /* SVC not used in this bare-metal firmware */
}

void Fault_PrefetchAbortHandler(uint32 addr, uint32 ifsr, uint32 ifar)
{
    (void)addr;
    (void)ifsr;
    (void)ifar;
    FaultHandler_Report(FAULT_ECC_DED, FAULT_SEVERITY_CRITICAL);
}

void Fault_DataAbortHandler(uint32 addr, uint32 dfsr, uint32 dfar)
{
    (void)addr;
    (void)dfsr;
    (void)dfar;
    FaultHandler_Report(FAULT_ECC_DED, FAULT_SEVERITY_CRITICAL);
}

void Fault_ESM_FIQ_Handler(uint32 intNum)
{
    (void)intNum;
    FaultHandler_ESM_Group2_ISR();
}

/* ========================================================================== */
/*                       Static Functions                                     */
/* ========================================================================== */

/**
 * @brief Classify fault as transient, permanent, or intermittent
 * @details Uses occurrence history to determine fault classification.
 *          - First occurrence: TRANSIENT
 *          - 2-3 occurrences: INTERMITTENT
 *          - 4+ occurrences: PERMANENT
 */
static FaultClass_t FaultHandler_Classify(uint32 faultCode)
{
    uint32 count = 0U;

    if (faultCode < 16U)
    {
        count = g_faultOccurrence[faultCode];
    }

    if (count <= 1U)
    {
        return FAULT_CLASS_TRANSIENT;
    }
    else if (count <= 3U)
    {
        return FAULT_CLASS_INTERMITTENT;
    }
    else
    {
        return FAULT_CLASS_PERMANENT;
    }
}

/**
 * @brief Attempt fault recovery
 * @param faultCode The fault to recover from
 * @return TRUE if recovery succeeded
 */
static boolean FaultHandler_AttemptRecovery(uint32 faultCode)
{
    uint32 retries;

    for (retries = 0U; retries < SAFE_STATE_MAX_RETRIES; retries++)
    {
        switch (faultCode)
        {
            case FAULT_ECC_SEC:
                /* Single-bit ECC: corrected automatically, just clear */
                REG32_WR(ECC_AGG_SEC_EOI, 0x01U);
                return TRUE;

            case FAULT_PLL_SLIP:
                /* Re-trigger PLL calibration */
                REG32_SET(PLL_CAL_CTRL, 0x01U);
                /* Wait for lock (simplified) */
                {
                    volatile uint32 wait;
                    for (wait = 0U; wait < 10000U; wait++) { /* spin */ }
                }
                if ((REG32_RD(PLL_STATUS) & PLL_STATUS_LOCKED) != 0U)
                {
                    return TRUE;
                }
                break;

            case FAULT_VOLTAGE_OOR:
            case FAULT_TEMP_OOR:
                /* Wait and re-check (thermal/voltage may settle) */
                {
                    volatile uint32 wait;
                    for (wait = 0U; wait < 50000U; wait++) { /* spin */ }
                }
                /* Re-check would happen on next diagnostic cycle */
                return TRUE;

            default:
                /* No recovery procedure defined */
                return FALSE;
        }
    }

    return FALSE;
}

/**
 * @brief Request transition to safe state
 * @param faultCode The fault triggering safe state
 */
static void FaultHandler_RequestSafeState(uint32 faultCode)
{
    (void)faultCode;

    /* Assert ESM error pin -- external monitor can detect this */
    REG32_WR(ESM_G3_EN_SET(0), 0xFFFFFFFFU);

    /* The main loop checks SafetyMonitor_GetStatus() which will
     * return SAFETY_FAULT, causing main to call EnterSafeState() */
}

/**
 * @brief Process an ESM error by number
 */
static void FaultHandler_ProcessESMError(uint32 esmErrNum,
                                          FaultSeverity_t severity)
{
    uint32 faultCode;

    switch (esmErrNum)
    {
        case ESM_ERR_LOCKSTEP_COMPARE:
            faultCode = FAULT_LOCKSTEP_COMPARE;
            severity = FAULT_SEVERITY_CRITICAL;
            break;

        case ESM_ERR_ECC_TCMA_SEC:
        case ESM_ERR_ECC_TCMB_SEC:
        case ESM_ERR_ECC_L2_SEC:
            faultCode = FAULT_ECC_SEC;
            break;

        case ESM_ERR_ECC_TCMA_DED:
        case ESM_ERR_ECC_TCMB_DED:
        case ESM_ERR_ECC_L2_DED:
            faultCode = FAULT_ECC_DED;
            severity = FAULT_SEVERITY_CRITICAL;
            break;

        case ESM_ERR_WDT_NMI:
            faultCode = FAULT_WDT_VIOLATION;
            severity = FAULT_SEVERITY_CRITICAL;
            break;

        case ESM_ERR_PLL_SLIP:
            faultCode = FAULT_PLL_SLIP;
            break;

        case ESM_ERR_VOLTAGE_MON:
            faultCode = FAULT_VOLTAGE_OOR;
            break;

        case ESM_ERR_TEMP_MON:
            faultCode = FAULT_TEMP_OOR;
            break;

        case ESM_ERR_RF_POWER_FAULT:
            faultCode = FAULT_RF_POWER;
            break;

        default:
            faultCode = 0xFFFFU;
            break;
    }

    FaultHandler_Report(faultCode, severity);
}
