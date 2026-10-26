/**
 * @file cache_ops.S
 * @brief ARM Assembly Cache Operations
 * @details Provides cache invalidate, clean, and flush operations for
 *          the Cortex-R5F L1 instruction and data caches. Required for
 *          DMA coherency and self-test memory operations.
 *
 * @copyright Copyright (c) 2024
 */

    .syntax unified
    .arch   armv7-r
    .arm

/* ========================================================================== */
/*  Invalidate Entire Instruction Cache                                       */
/*  void Cache_InvalidateICache(void);                                        */
/* ========================================================================== */

    .section .text, "ax"
    .global Cache_InvalidateICache
    .type   Cache_InvalidateICache, %function
    .align  4

Cache_InvalidateICache:
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c5, 0      /* ICIALLU: Invalidate all I-Cache  */
    DSB
    ISB
    BX      LR

/* ========================================================================== */
/*  Invalidate Entire Data Cache                                              */
/*  void Cache_InvalidateDCache(void);                                        */
/* ========================================================================== */

    .global Cache_InvalidateDCache
    .type   Cache_InvalidateDCache, %function
    .align  4

Cache_InvalidateDCache:
    MOV     R0, #0
    MCR     p15, 0, R0, c7, c6, 0      /* DCISW: Invalidate all D-Cache    */
    DSB
    ISB
    BX      LR

/* ========================================================================== */
/*  Clean Entire Data Cache (write-back dirty lines)                          */
/*  void Cache_CleanDCache(void);                                             */
/* ========================================================================== */

    .global Cache_CleanDCache
    .type   Cache_CleanDCache, %function
    .align  4

Cache_CleanDCache:
    /* Clean by set/way -- iterate over all cache sets and ways */
    MRC     p15, 1, R0, c0, c0, 0      /* Read CCSIDR (cache size)         */

    /* Extract set and way information */
    AND     R1, R0, #0x07              /* Line size field                   */
    ADD     R1, R1, #4                 /* Line size = 2^(field + 4)         */

    LDR     R3, =0x3FF
    AND     R2, R3, R0, LSR #3        /* Number of ways - 1                */

    LDR     R3, =0x7FFF
    AND     R0, R3, R0, LSR #13       /* Number of sets - 1                */

    CLZ     R4, R2                     /* Way shift amount                  */

clean_set_loop:
    MOV     R5, R2                     /* Working copy of ways              */

clean_way_loop:
    ORR     R6, R0, R5, LSL R4        /* Set/way value                     */
    ORR     R6, R6, R0, LSL R1
    MCR     p15, 0, R6, c7, c10, 2    /* DCCSW: Clean by set/way           */
    SUBS    R5, R5, #1
    BGE     clean_way_loop

    SUBS    R0, R0, #1
    BGE     clean_set_loop

    DSB
    ISB
    BX      LR

/* ========================================================================== */
/*  Clean and Invalidate Data Cache by Address Range                          */
/*  void Cache_CleanInvalidateRange(uint32 startAddr, uint32 size);           */
/* ========================================================================== */

    .global Cache_CleanInvalidateRange
    .type   Cache_CleanInvalidateRange, %function
    .align  4

Cache_CleanInvalidateRange:
    /* R0 = start address, R1 = size (bytes) */
    ADD     R1, R0, R1                 /* R1 = end address                  */
    BIC     R0, R0, #31               /* Align to cache line (32 bytes)    */

ci_range_loop:
    MCR     p15, 0, R0, c7, c14, 1    /* DCCIMVAC: Clean & Invalidate by VA*/
    ADD     R0, R0, #32               /* Next cache line                   */
    CMP     R0, R1
    BLT     ci_range_loop

    DSB
    ISB
    BX      LR

/* ========================================================================== */
/*  Invalidate Data Cache by Address Range                                    */
/*  void Cache_InvalidateRange(uint32 startAddr, uint32 size);                */
/* ========================================================================== */

    .global Cache_InvalidateRange
    .type   Cache_InvalidateRange, %function
    .align  4

Cache_InvalidateRange:
    ADD     R1, R0, R1
    BIC     R0, R0, #31

inv_range_loop:
    MCR     p15, 0, R0, c7, c6, 1     /* DCIMVAC: Invalidate by VA         */
    ADD     R0, R0, #32
    CMP     R0, R1
    BLT     inv_range_loop

    DSB
    ISB
    BX      LR

/* ========================================================================== */
/*  Disable All Caches                                                        */
/*  void Cache_DisableAll(void);                                              */
/* ========================================================================== */

    .global Cache_DisableAll
    .type   Cache_DisableAll, %function
    .align  4

Cache_DisableAll:
    MRC     p15, 0, R0, c1, c0, 0     /* Read SCTLR                        */
    BIC     R0, R0, #(1 << 2)         /* Clear C bit (D-Cache)             */
    BIC     R0, R0, #(1 << 12)        /* Clear I bit (I-Cache)             */
    MCR     p15, 0, R0, c1, c0, 0     /* Write SCTLR                       */
    DSB
    ISB
    BX      LR

    .end
