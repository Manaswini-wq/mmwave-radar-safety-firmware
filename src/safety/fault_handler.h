/**
 * @file fault_handler.h
 * @brief Fault Handler Module Interface
 * @details ESM interrupt handling, fault classification, recovery logic,
 *          and safe-state entry for ISO 26262 ASIL-D compliance.
 *
 * @copyright Copyright (c) 2024
 */
#ifndef FAULT_HANDLER_H
#define FAULT_HANDLER_H

#include "platform_types.h"

/* ========================================================================== */
/*                           Fault Codes                                      */
/* ========================================================================== */

#define FAULT_NONE               0x0000U
#define FAULT_LOCKSTEP_COMPARE   0x0001U
#define FAULT_ECC_SEC            0x0002U
#define FAULT_ECC_DED            0x0003U
#define FAULT_WDT_VIOLATION      0x0004U
#define FAULT_PLL_SLIP           0x0005U
#define FAULT_VOLTAGE_OOR        0x0006U
#define FAULT_TEMP_OOR           0x0007U
#define FAULT_DMA_ERROR          0x0008U
#define FAULT_VIM_PARITY         0x0009U
#define FAULT_RF_POWER           0x000AU
#define FAULT_DATA_INTEGRITY     0x000BU
#define FAULT_DIAGNOSTIC_FAIL    0x000CU
#define FAULT_STACK_OVERFLOW     0x000DU
#define FAULT_CFM_VIOLATION      0x000EU

/* ========================================================================== */
/*                           Fault Severity                                   */
/* ========================================================================== */

typedef enum {
    FAULT_SEVERITY_LOW      = 0U,   /**< Informational, no action required    */
    FAULT_SEVERITY_MEDIUM   = 1U,   /**< Recovery attempt, then monitor       */
    FAULT_SEVERITY_HIGH     = 2U,   /**< Immediate recovery, safe-state if fail */
    FAULT_SEVERITY_CRITICAL = 3U    /**< Immediate safe-state entry           */
} FaultSeverity_t;

/* ========================================================================== */
/*                           Fault Classification                             */
/* ========================================================================== */

typedef enum {
    FAULT_CLASS_TRANSIENT   = 0U,   /**< Soft error, recoverable             */
    FAULT_CLASS_PERMANENT   = 1U,   /**< Hard error, not recoverable         */
    FAULT_CLASS_INTERMITTENT = 2U   /**< Recurring transient                 */
} FaultClass_t;

/* ========================================================================== */
/*                           Fault Log Entry                                  */
/* ========================================================================== */

typedef struct {
    uint32          faultCode;      /**< Fault identification code           */
    uint32          timestamp;      /**< System tick at detection            */
    FaultSeverity_t severity;       /**< Fault severity level                */
    FaultClass_t    classification; /**< Transient / permanent / intermittent*/
    uint32          recoveryCount;  /**< Number of recovery attempts         */
    boolean         resolved;       /**< TRUE if fault was recovered         */
} FaultLogEntry_t;

/* ========================================================================== */
/*                           Public API                                       */
/* ========================================================================== */

Status_t FaultHandler_Init(void);
void FaultHandler_Report(uint32 faultCode, FaultSeverity_t severity);
void FaultHandler_LogFault(uint32 faultCode, uint32 timestamp);
void FaultHandler_ESM_Group1_ISR(void);
void FaultHandler_ESM_Group2_ISR(void);

/* Called from vector_table.S exception handlers */
void Fault_UndefinedHandler(uint32 addr, uint32 *pContext);
void Fault_SVCHandler(uint32 svcNum, uint32 *pContext);
void Fault_PrefetchAbortHandler(uint32 addr, uint32 ifsr, uint32 ifar);
void Fault_DataAbortHandler(uint32 addr, uint32 dfsr, uint32 dfar);
void Fault_ESM_FIQ_Handler(uint32 intNum);

#endif /* FAULT_HANDLER_H */
