/**
 * @file ecc_handler.c
 * @brief ECC Handler Implementation
 * @details Manages ECC (Error Correction Code) for all on-chip memories.
 *          - SEC (Single Error Correction): corrects single-bit errors,
 *            logs occurrence, and alerts if threshold exceeded.
 *          - DED (Double Error Detection): uncorrectable, triggers safe state.
 *          - Error injection for self-test verification.
 *
 *          ISO 26262 Table 11, 2: Memory error detection
 *          IEC 61508 Table A.10: Error detecting / correcting codes
 *
 * @copyright Copyright (c) 2024
 */

#include "ecc_handler.h"
#include "safety_cfg.h"
#include "awr2944_regs.h"
#include "fault_handler.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief SEC error counters per memory region */
static uint32 g_secCount_TCMA = 0U;
static uint32 g_secCount_TCMB = 0U;
static uint32 g_secCount_L2   = 0U;
static uint32 g_secCountTotal = 0U;

/** @brief DED error counters */
static uint32 g_dedCount = 0U;

/** @brief Module initialized flag */
static boolean g_eccInitialized = FALSE;

/* Memory region IDs */
#define ECC_REGION_TCMA    0U
#define ECC_REGION_TCMB    1U
#define ECC_REGION_L2      2U
#define ECC_REGION_VIM     3U

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize ECC for all memory regions
 * @details Enables ECC checking and correction on TCMA, TCMB, L2RAM,
 *          and VIM RAM. Memories must have been initialized (written)
 *          before ECC is enabled to avoid false DED errors.
 * @return STATUS_OK on success
 */
Status_t ECC_Init(void)
{
    /* Enable ECC aggregator */
    REG32_WR(ECC_AGG_CTRL, ECC_CTRL_ENABLE | ECC_CTRL_CHECK_ENABLE);

    /* Enable ECC for TCMA */
    if (ECC_TCMA_ENABLE == TRUE)
    {
        /* TCMA ECC is enabled via R5F core configuration */
        uint32 ctrl = REG32_RD(MSS_CTRL_R5_CONTROL);
        ctrl |= (1U << 3);  /* ECC enable for ATCM */
        REG32_WR(MSS_CTRL_R5_CONTROL, ctrl);
    }

    /* Enable ECC for TCMB */
    if (ECC_TCMB_ENABLE == TRUE)
    {
        uint32 ctrl = REG32_RD(MSS_CTRL_R5_CONTROL);
        ctrl |= (1U << 4);  /* ECC enable for BTCM */
        REG32_WR(MSS_CTRL_R5_CONTROL, ctrl);
    }

    /* Enable ECC for L2 SRAM */
    if (ECC_L2_ENABLE == TRUE)
    {
        /* L2 ECC is controlled by the ECC aggregator */
        REG32_WR(ECC_AGG_ERR_CTRL1, 0x01U);
    }

    /* Clear any pending ECC status */
    REG32_WR(ECC_AGG_ERR_STATUS1, 0xFFFFFFFFU);
    REG32_WR(ECC_AGG_ERR_STATUS2, 0xFFFFFFFFU);
    REG32_WR(ECC_AGG_SEC_EOI, 0x01U);
    REG32_WR(ECC_AGG_DED_EOI, 0x01U);

    /* Reset counters */
    g_secCount_TCMA = 0U;
    g_secCount_TCMB = 0U;
    g_secCount_L2   = 0U;
    g_secCountTotal = 0U;
    g_dedCount      = 0U;

    g_eccInitialized = TRUE;

    return STATUS_OK;
}

/**
 * @brief ECC self-test via error injection
 * @details Injects a single-bit error into a test location and verifies
 *          that the ECC mechanism detects and corrects it. This validates
 *          the ECC hardware is functional.
 * @return DIAG_PASS if SEC detection works correctly
 */
DiagResult_t ECC_SelfTest(void)
{
    uint32 savedStatus;
    uint32 testAddr;
    volatile uint32 readBack;

    if (g_eccInitialized != TRUE)
    {
        return DIAG_FAIL;
    }

    /* Use BIST data section for test to avoid corrupting live data */
    extern uint32 _bist_data_start;
    testAddr = (uint32)&_bist_data_start;

    /* Write known value */
    *((volatile uint32 *)testAddr) = 0xA5A5A5A5U;

    /* Enable error injection (single-bit) */
    REG32_WR(ECC_AGG_ERR_CTRL1, ECC_CTRL_ERR_INJECT);
    REG32_WR(ECC_AGG_ERR_CTRL2, 0x01U);  /* 1-bit flip */

    /* Read the location -- this should trigger SEC correction */
    readBack = *((volatile uint32 *)testAddr);

    /* Disable error injection */
    REG32_CLR(ECC_AGG_ERR_CTRL1, ECC_CTRL_ERR_INJECT);

    /* Check that SEC status was set */
    savedStatus = REG32_RD(ECC_AGG_ERR_STATUS1);

    /* Clear SEC status */
    REG32_WR(ECC_AGG_SEC_EOI, 0x01U);
    REG32_WR(ECC_AGG_ERR_STATUS1, savedStatus);

    if ((savedStatus & ECC_SEC_ERROR_BIT) == 0U)
    {
        /* SEC was not detected -- ECC hardware fault */
        return DIAG_FAIL;
    }

    /* Verify the corrected read value matches original */
    if (readBack != 0xA5A5A5A5U)
    {
        return DIAG_FAIL;
    }

    return DIAG_PASS;
}

/**
 * @brief Get ECC diagnostic status
 * @return DIAG_PASS if SEC count is below threshold and no DED detected
 */
DiagResult_t ECC_GetDiagStatus(void)
{
    /* Any DED is a critical failure */
    if (g_dedCount > 0U)
    {
        return DIAG_FAIL;
    }

    /* Check SEC threshold */
    if (g_secCountTotal > ECC_SEC_THRESHOLD)
    {
        return DIAG_FAIL;
    }

    return DIAG_PASS;
}

/**
 * @brief SEC (Single Error Correction) interrupt handler
 * @details Called when ECC detects and corrects a single-bit error.
 *          Increments counters and checks threshold for escalation.
 */
void ECC_SEC_ISR(void)
{
    uint32 status = REG32_RD(ECC_AGG_ERR_STATUS1);

    /* Determine which memory region had the SEC */
    uint32 secSource = REG32_RD(ECC_AGG_SEC_STATUS(0));

    if ((secSource & 0x01U) != 0U)
    {
        g_secCount_TCMA++;
    }
    if ((secSource & 0x02U) != 0U)
    {
        g_secCount_TCMB++;
    }
    if ((secSource & 0x04U) != 0U)
    {
        g_secCount_L2++;
    }

    g_secCountTotal = g_secCount_TCMA + g_secCount_TCMB + g_secCount_L2;

    /* Clear SEC status */
    REG32_WR(ECC_AGG_SEC_EOI, 0x01U);
    REG32_WR(ECC_AGG_ERR_STATUS1, status);

    /* Check if SEC threshold exceeded */
    if (g_secCountTotal >= ECC_SEC_THRESHOLD)
    {
        FaultHandler_Report(FAULT_ECC_SEC, FAULT_SEVERITY_HIGH);
    }
}

/**
 * @brief DED (Double Error Detection) interrupt handler
 * @details Called when an uncorrectable double-bit error is detected.
 *          This is a critical fault requiring immediate safe-state entry.
 */
void ECC_DED_ISR(void)
{
    uint32 status = REG32_RD(ECC_AGG_ERR_STATUS2);

    g_dedCount++;

    /* Clear DED status */
    REG32_WR(ECC_AGG_DED_EOI, 0x01U);
    REG32_WR(ECC_AGG_ERR_STATUS2, status);

    /* DED is always critical */
    FaultHandler_Report(FAULT_ECC_DED, FAULT_SEVERITY_CRITICAL);
}

/**
 * @brief Get total SEC error count
 */
uint32 ECC_GetSECCount(void)
{
    return g_secCountTotal;
}

/**
 * @brief Get DED error count
 */
uint32 ECC_GetDEDCount(void)
{
    return g_dedCount;
}

/**
 * @brief Inject an ECC error for testing
 * @param memRegion Memory region ID
 * @param doubleBit TRUE for DED injection, FALSE for SEC
 * @return STATUS_OK if injection was configured
 */
Status_t ECC_InjectError(uint32 memRegion, boolean doubleBit)
{
    (void)memRegion;

    uint32 numBits = (doubleBit == TRUE) ? 0x02U : 0x01U;

    REG32_WR(ECC_AGG_ERR_CTRL2, numBits);
    REG32_SET(ECC_AGG_ERR_CTRL1, ECC_CTRL_ERR_INJECT);

    return STATUS_OK;
}
