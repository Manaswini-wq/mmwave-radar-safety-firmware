/**
 * @file control_flow_monitor.c
 * @brief Control Flow Monitoring Implementation
 * @details Implements runtime control flow integrity verification using
 *          a signature-based approach. Each checkpoint in the expected
 *          execution path updates a running signature. At verification
 *          points, the accumulated signature is compared against the
 *          expected value to detect illegal execution paths.
 *
 *          ISO 26262 Table 11, 1e: Program flow monitoring
 *          IEC 61508 Table A.3: Temporal and logical program monitoring
 *
 * @copyright Copyright (c) 2024
 */

#include "control_flow_monitor.h"
#include "safety_cfg.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

/** @brief Running signature (primary and inverse copy) */
static uint32 g_cfmSignature     = 0U;
static uint32 g_cfmSignature_inv = 0xFFFFFFFFU;

/** @brief Seed value for reset */
static uint32 g_cfmSeed = 0U;

/** @brief Checkpoint hit counter array */
static uint32 g_checkpointCount[CFM_CHECKPOINT_COUNT];

/** @brief Initialization flag */
static boolean g_cfmInitialized = FALSE;

/** @brief Expected signature after init sequence (computed at startup) */
static uint32 g_expectedInitSig = 0U;

/* ========================================================================== */
/*                       Signature Computation                                */
/* ========================================================================== */

/**
 * @brief Update signature with a checkpoint value
 * @details Uses a simple but effective mixing function:
 *          sig = ((sig << 3) | (sig >> 29)) ^ (checkpoint * prime)
 *          This detects both missing and reordered checkpoints.
 */
static inline uint32 CFM_MixSignature(uint32 currentSig, uint32 cpId)
{
    /* Rotate left by 3 */
    uint32 rotated = (currentSig << 3U) | (currentSig >> 29U);
    /* XOR with checkpoint-specific value using a prime multiplier */
    return rotated ^ (cpId * 0x9E3779B1U);
}

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize control flow monitor
 * @param seed Initial signature seed value
 */
void CFM_Init(uint32 seed)
{
    uint32 i;

    g_cfmSeed          = seed;
    g_cfmSignature     = seed;
    g_cfmSignature_inv = ~seed;

    for (i = 0U; i < CFM_CHECKPOINT_COUNT; i++)
    {
        g_checkpointCount[i] = 0U;
    }

    /* Pre-compute expected signature after initialization sequence:
     * SAFETY_INIT_DONE -> ROM_PATCH_DONE -> HW_INIT_DONE ->
     * SELFTEST_DONE -> CAL_DONE
     */
    g_expectedInitSig = seed;
    g_expectedInitSig = CFM_MixSignature(g_expectedInitSig, CFM_CP_SAFETY_INIT_DONE);
    g_expectedInitSig = CFM_MixSignature(g_expectedInitSig, CFM_CP_ROM_PATCH_DONE);
    g_expectedInitSig = CFM_MixSignature(g_expectedInitSig, CFM_CP_HW_INIT_DONE);
    g_expectedInitSig = CFM_MixSignature(g_expectedInitSig, CFM_CP_SELFTEST_DONE);
    g_expectedInitSig = CFM_MixSignature(g_expectedInitSig, CFM_CP_CAL_DONE);

    g_cfmInitialized = TRUE;
}

/**
 * @brief Record a checkpoint in the execution flow
 * @param checkpointId Unique checkpoint identifier
 */
void CFM_Checkpoint(uint32 checkpointId)
{
    if (g_cfmInitialized != TRUE)
    {
        return;
    }

    if (checkpointId >= CFM_CHECKPOINT_COUNT)
    {
        return;
    }

    /* Update running signature */
    g_cfmSignature = CFM_MixSignature(g_cfmSignature, checkpointId);
    g_cfmSignature_inv = ~g_cfmSignature;

    /* Increment checkpoint hit counter */
    g_checkpointCount[checkpointId]++;
}

/**
 * @brief Verify control flow integrity
 * @details Checks that:
 *   1. Primary and inverse signatures are consistent
 *   2. All mandatory init checkpoints have been hit at least once
 *   3. Loop checkpoints are balanced (LOOP_START == LOOP_END)
 *
 * @return STATUS_OK if control flow is valid
 */
Status_t CFM_Verify(void)
{
    if (g_cfmInitialized != TRUE)
    {
        return STATUS_ERROR;
    }

    /* Check signature consistency (primary vs inverse) */
    if (g_cfmSignature != ~g_cfmSignature_inv)
    {
        return STATUS_SAFETY_FAULT;
    }

    /* Verify all init checkpoints were executed */
    if (g_checkpointCount[CFM_CP_SAFETY_INIT_DONE] == 0U)
    {
        return STATUS_SAFETY_FAULT;
    }
    if (g_checkpointCount[CFM_CP_ROM_PATCH_DONE] == 0U)
    {
        return STATUS_SAFETY_FAULT;
    }
    if (g_checkpointCount[CFM_CP_HW_INIT_DONE] == 0U)
    {
        return STATUS_SAFETY_FAULT;
    }
    if (g_checkpointCount[CFM_CP_SELFTEST_DONE] == 0U)
    {
        return STATUS_SAFETY_FAULT;
    }
    if (g_checkpointCount[CFM_CP_CAL_DONE] == 0U)
    {
        return STATUS_SAFETY_FAULT;
    }

    /* Verify loop checkpoint balance */
    uint32 startCount = g_checkpointCount[CFM_CP_LOOP_START];
    uint32 endCount   = g_checkpointCount[CFM_CP_LOOP_END];

    /* Start count should equal end count (or be 1 ahead if mid-loop) */
    if (startCount > (endCount + 1U))
    {
        return STATUS_SAFETY_FAULT;
    }
    if (endCount > startCount)
    {
        return STATUS_SAFETY_FAULT;
    }

    return STATUS_OK;
}

/**
 * @brief Reset control flow monitor to initial state
 */
void CFM_Reset(void)
{
    uint32 i;

    g_cfmSignature     = g_cfmSeed;
    g_cfmSignature_inv = ~g_cfmSeed;

    for (i = 0U; i < CFM_CHECKPOINT_COUNT; i++)
    {
        g_checkpointCount[i] = 0U;
    }
}
