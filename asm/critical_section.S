/**
 * @file critical_section.S
 * @brief ARM Assembly Critical Section Primitives
 * @details Provides interrupt-safe critical section enter/exit with
 *          CPSR (Current Program Status Register) state save/restore.
 *          Used throughout safety-critical code to protect shared data.
 *
 *          ISO 26262 ASIL-D: Critical sections must be bounded in time
 *          to not violate watchdog and diagnostic timing requirements.
 *
 * @copyright Copyright (c) 2024
 */

    .syntax unified
    .arch   armv7-r
    .arm

/* ========================================================================== */
/*  Critical Section Enter                                                    */
/*  Disables IRQ and FIQ, returns previous CPSR for later restore.            */
/*  uint32 CriticalSection_Enter(void);                                       */
/* ========================================================================== */

    .section .text, "ax"
    .global CriticalSection_Enter
    .type   CriticalSection_Enter, %function
    .align  4

CriticalSection_Enter:
    MRS     R0, CPSR            /* R0 = current CPSR (saved for return)     */
    CPSID   IF                  /* Disable IRQ (I) and FIQ (F)              */
    DSB                         /* Ensure all memory accesses complete       */
    ISB                         /* Flush pipeline                            */
    BX      LR                  /* Return saved CPSR in R0                   */

/* ========================================================================== */
/*  Critical Section Exit                                                     */
/*  Restores CPSR to the value saved by CriticalSection_Enter.                */
/*  void CriticalSection_Exit(uint32 savedCpsr);                              */
/* ========================================================================== */

    .global CriticalSection_Exit
    .type   CriticalSection_Exit, %function
    .align  4

CriticalSection_Exit:
    DSB                         /* Ensure all memory accesses complete       */
    MSR     CPSR_c, R0          /* Restore CPSR control field (I, F bits)    */
    ISB                         /* Flush pipeline                            */
    BX      LR

/* ========================================================================== */
/*  Data Memory Barrier                                                       */
/*  void DMB(void);                                                           */
/* ========================================================================== */

    .global DMB_Instruction
    .type   DMB_Instruction, %function
    .align  4

DMB_Instruction:
    DMB
    BX      LR

/* ========================================================================== */
/*  Data Synchronization Barrier                                              */
/*  void DSB(void);                                                           */
/* ========================================================================== */

    .global DSB_Instruction
    .type   DSB_Instruction, %function
    .align  4

DSB_Instruction:
    DSB
    BX      LR

/* ========================================================================== */
/*  Instruction Synchronization Barrier                                       */
/*  void ISB(void);                                                           */
/* ========================================================================== */

    .global ISB_Instruction
    .type   ISB_Instruction, %function
    .align  4

ISB_Instruction:
    ISB
    BX      LR

/* ========================================================================== */
/*  Wait For Interrupt (low-power idle)                                       */
/*  void WFI(void);                                                           */
/* ========================================================================== */

    .global WFI_Instruction
    .type   WFI_Instruction, %function
    .align  4

WFI_Instruction:
    DSB                         /* Ensure pending writes complete             */
    WFI                         /* Enter idle until next interrupt            */
    BX      LR

    .end
