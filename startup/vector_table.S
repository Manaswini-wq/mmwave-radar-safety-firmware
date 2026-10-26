/**
 * @file vector_table.S
 * @brief ARM Cortex-R5F Exception Vector Table
 * @details Bare-metal vector table for TI AWR2944 MSS R5F core.
 *          Placed at address 0x00000000 (TCMA base) via linker section .vectors.
 *
 *          ISO 26262 ASIL-D: All exception vectors are populated;
 *          unexpected exceptions trigger safe-state entry.
 *
 * @copyright Copyright (c) 2024
 */

    .syntax unified
    .arch   armv7-r
    .arm

/* ========================================================================== */
/*                         Vector Table                                       */
/* ========================================================================== */

    .section .vectors, "ax"
    .global _resetvectors
    .type   _resetvectors, %function

_resetvectors:
    LDR     PC, _reset_addr         /* 0x00: Reset                           */
    LDR     PC, _undef_addr         /* 0x04: Undefined Instruction           */
    LDR     PC, _svc_addr           /* 0x08: Supervisor Call (SVC)           */
    LDR     PC, _pabt_addr          /* 0x0C: Prefetch Abort                  */
    LDR     PC, _dabt_addr          /* 0x10: Data Abort                      */
    LDR     PC, _rsvd_addr          /* 0x14: Reserved                        */
    LDR     PC, _irq_addr           /* 0x18: IRQ                             */
    LDR     PC, _fiq_addr           /* 0x1C: FIQ                             */

/* Exception handler address table */
_reset_addr:    .word   _c_int00
_undef_addr:    .word   _undef_handler
_svc_addr:      .word   _svc_handler
_pabt_addr:     .word   _pabt_handler
_dabt_addr:     .word   _dabt_handler
_rsvd_addr:     .word   _rsvd_handler
_irq_addr:      .word   _irq_handler
_fiq_addr:      .word   _fiq_handler

/* ========================================================================== */
/*                      Reset Handler                                         */
/* ========================================================================== */

    .section .text, "ax"
    .global _c_int00
    .type   _c_int00, %function
    .align  4

_c_int00:
    /* Disable IRQ and FIQ at entry */
    CPSID   IF

    /* Invalidate caches and TLB */
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c5, 0      /* Invalidate I-Cache       */
    MCR     p15, 0, R0, c7, c6, 0      /* Invalidate D-Cache       */
    MCR     p15, 0, R0, c8, c7, 0      /* Invalidate TLB           */
    DSB
    ISB

    /* Set up SVC mode stack */
    CPS     #0x13                       /* SVC mode                 */
    LDR     SP, =_svc_stack_end

    /* Set up IRQ mode stack */
    CPS     #0x12                       /* IRQ mode                 */
    LDR     SP, =_irq_stack_end

    /* Set up FIQ mode stack */
    CPS     #0x11                       /* FIQ mode                 */
    LDR     SP, =_fiq_stack_end

    /* Set up Abort mode stack */
    CPS     #0x17                       /* Abort mode               */
    LDR     SP, =_abort_stack_end

    /* Set up Undefined mode stack */
    CPS     #0x1B                       /* Undefined mode           */
    LDR     SP, =_undef_stack_end

    /* Set up System/User mode stack (main application stack) */
    CPS     #0x1F                       /* System mode              */
    LDR     SP, =_sys_stack_end

    /* Switch to SVC mode for startup */
    CPS     #0x13

    /* Branch to C startup code */
    BL      _startup_init

    /* Branch to main -- should never return */
    BL      main

    /* If main returns, enter safe state (should never happen) */
_main_returned:
    CPSID   IF
    B       _main_returned

/* ========================================================================== */
/*                      Undefined Instruction Handler                         */
/* ========================================================================== */

    .global _undef_handler
    .type   _undef_handler, %function
    .align  4

_undef_handler:
    /* Save context */
    STMFD   SP!, {R0-R12, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Call C handler -- passes fault address in LR (adjusted) */
    SUB     R0, LR, #4             /* R0 = faulting instruction address     */
    MOV     R1, SP                 /* R1 = saved context pointer            */
    BL      Fault_UndefinedHandler

    /* Restore context */
    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R12, PC}^

/* ========================================================================== */
/*                      SVC Handler                                           */
/* ========================================================================== */

    .global _svc_handler
    .type   _svc_handler, %function
    .align  4

_svc_handler:
    STMFD   SP!, {R0-R12, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Extract SVC number from instruction */
    LDR     R0, [LR, #-4]
    BIC     R0, R0, #0xFF000000    /* R0 = SVC number                      */
    MOV     R1, SP
    BL      Fault_SVCHandler

    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R12, PC}^

/* ========================================================================== */
/*                      Prefetch Abort Handler                                */
/* ========================================================================== */

    .global _pabt_handler
    .type   _pabt_handler, %function
    .align  4

_pabt_handler:
    SUB     LR, LR, #4            /* Adjust return address                 */
    STMFD   SP!, {R0-R12, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Read Instruction Fault Status Register */
    MRC     p15, 0, R1, c5, c0, 1  /* R1 = IFSR                           */
    MRC     p15, 0, R2, c6, c0, 2  /* R2 = IFAR                           */
    MOV     R0, LR                  /* R0 = faulting address               */
    BL      Fault_PrefetchAbortHandler

    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R12, PC}^

/* ========================================================================== */
/*                      Data Abort Handler                                    */
/* ========================================================================== */

    .global _dabt_handler
    .type   _dabt_handler, %function
    .align  4

_dabt_handler:
    SUB     LR, LR, #8            /* Adjust return address                 */
    STMFD   SP!, {R0-R12, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Read Data Fault Status Register */
    MRC     p15, 0, R1, c5, c0, 0  /* R1 = DFSR                           */
    MRC     p15, 0, R2, c6, c0, 0  /* R2 = DFAR                           */
    MOV     R0, LR                  /* R0 = faulting address               */
    BL      Fault_DataAbortHandler

    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R12, PC}^

/* ========================================================================== */
/*                      IRQ Handler                                           */
/* ========================================================================== */

    .global _irq_handler
    .type   _irq_handler, %function
    .align  4

_irq_handler:
    SUB     LR, LR, #4
    STMFD   SP!, {R0-R12, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Read active IRQ number from VIM */
    LDR     R0, =0x50F00020        /* VIM_ACTIRQ                          */
    LDR     R0, [R0]
    AND     R0, R0, #0xFF          /* Extract interrupt number             */

    /* Call C IRQ dispatcher */
    BL      HAL_Interrupt_Dispatch

    /* Acknowledge interrupt in VIM */
    LDR     R1, =0x50F00020        /* VIM_ACTIRQ                          */
    MOV     R2, #0
    STR     R2, [R1]               /* Clear active IRQ                    */

    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R12, PC}^

/* ========================================================================== */
/*                      FIQ Handler                                           */
/* ========================================================================== */

    .global _fiq_handler
    .type   _fiq_handler, %function
    .align  4

_fiq_handler:
    SUB     LR, LR, #4
    /* FIQ has banked R8-R14, minimal save needed */
    STMFD   SP!, {R0-R7, LR}
    MRS     R0, SPSR
    STMFD   SP!, {R0}

    /* Read active FIQ from VIM */
    LDR     R0, =0x50F00024        /* VIM_ACTFIQ                          */
    LDR     R0, [R0]
    AND     R0, R0, #0xFF

    /* FIQ is used for safety-critical ESM Group 2 interrupts */
    BL      Fault_ESM_FIQ_Handler

    /* Acknowledge */
    LDR     R1, =0x50F00024
    MOV     R2, #0
    STR     R2, [R1]

    LDMFD   SP!, {R0}
    MSR     SPSR_cxsf, R0
    LDMFD   SP!, {R0-R7, PC}^

/* ========================================================================== */
/*                      Reserved Vector Handler                               */
/* ========================================================================== */

    .global _rsvd_handler
    .type   _rsvd_handler, %function
    .align  4

_rsvd_handler:
    /* Reserved vector should never be entered; enter infinite loop.
     * In a safety system this indicates memory corruption. */
    CPSID   IF
    B       _rsvd_handler

    .end
