/**
 * @file safety_monitor.h
 * @brief Safety Monitor Module Interface
 * @details Periodic diagnostic scheduler, SPFM/LFM calculation,
 *          safe-state transition logic, and safety mechanism status tracking.
 *          Implements ISO 26262 ASIL-D and IEC 61508 SIL-3 monitoring.
 *
 * @copyright Copyright (c) 2024
 */
#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include "platform_types.h"

/* ========================================================================== */
/*                           Diagnostic IDs                                   */
/* ========================================================================== */

#define DIAG_ID_CPU_REG          0U
#define DIAG_ID_STACK_OVERFLOW   1U
#define DIAG_ID_VOLTAGE_MON      2U
#define DIAG_ID_TEMP_MON         3U
#define DIAG_ID_PLL_LOCK         4U
#define DIAG_ID_ECC_STATUS       5U
#define DIAG_ID_WDT_CHECK        6U
#define DIAG_ID_RAM_MARCH        7U
#define DIAG_ID_ADC_LOOPBACK     8U
#define DIAG_ID_DMA_TEST         9U
#define DIAG_ID_CFM_CHECK        10U
#define DIAG_ID_LOCKSTEP_CHECK   11U
#define DIAG_ID_COUNT            12U

/* ========================================================================== */
/*                           Type Definitions                                 */
/* ========================================================================== */

/** @brief Diagnostic test entry */
typedef struct {
    uint32      diagId;            /**< Diagnostic identifier               */
    uint32      intervalMs;        /**< Execution interval in milliseconds  */
    uint32      lastRunMs;         /**< Timestamp of last execution         */
    uint32      passCount;         /**< Cumulative pass count               */
    uint32      failCount;         /**< Cumulative fail count               */
    DiagResult_t lastResult;       /**< Most recent test result             */
    boolean     enabled;           /**< Enable flag                         */
} DiagEntry_t;

/** @brief Safety metrics snapshot */
typedef struct {
    uint32  spfmPercent;           /**< Single-Point Fault Metric (%)       */
    uint32  lfmPercent;            /**< Latent Fault Metric (%)             */
    uint32  sffPercent;            /**< Safe Failure Fraction (%)           */
    uint32  totalFaults;           /**< Total faults detected               */
    uint32  safeStateCount;        /**< Number of safe-state entries        */
    boolean isOperational;         /**< TRUE if system is operational       */
} SafetyMetrics_t;

/* ========================================================================== */
/*                           Public API                                       */
/* ========================================================================== */

/**
 * @brief Initialize the safety monitor
 * @return STATUS_OK on success
 */
Status_t SafetyMonitor_Init(void);

/**
 * @brief Run a specific diagnostic test
 * @param diagId Diagnostic test identifier
 * @return STATUS_OK if test passed
 */
Status_t SafetyMonitor_RunDiagnostic(uint32 diagId);

/**
 * @brief Run all enabled diagnostics that are due
 * @param currentTimeMs Current system time in milliseconds
 */
void SafetyMonitor_RunScheduled(uint32 currentTimeMs);

/**
 * @brief Get overall safety monitor status
 * @return STATUS_OK if all diagnostics are passing
 */
Status_t SafetyMonitor_GetStatus(void);

/**
 * @brief Get current safety metrics
 * @param pMetrics Pointer to metrics output structure
 * @return STATUS_OK on success
 */
Status_t SafetyMonitor_GetMetrics(SafetyMetrics_t *pMetrics);

/**
 * @brief Report a diagnostic failure
 * @param diagId Diagnostic that failed
 * @param result Failure result code
 */
void SafetyMonitor_ReportFailure(uint32 diagId, DiagResult_t result);

#endif /* SAFETY_MONITOR_H */
