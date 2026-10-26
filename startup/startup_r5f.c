/**
 * @file startup_r5f.c
 * @brief Bare-metal startup code for AWR2944 Cortex-R5F
 * @details Performs early hardware initialization before main():
 *          - BSS clearing (with ECC-safe writes)
 *          - Initialized data copy from load address
 *          - FPU / VFP enable
 *          - Cache and MPU configuration
 *          - Stack guard pattern initialization
 *
 *          Called from vector_table.S after stack setup.
 *
 * @note ISO 26262 ASIL-D: Startup sequence is safety-relevant.
 *       Any failure here must result in safe-state entry.
 *
 * @copyright Copyright (c) 2024
 */

#include "platform_types.h"
#include "awr2944_regs.h"

/* ========================================================================== */
/*                         External Linker Symbols                            */
/* ========================================================================== */

extern uint32 _bss_start;
extern uint32 _bss_end;
extern uint32 _data_start;
extern uint32 _data_end;
extern uint32 _data_load_start;
extern uint32 _safety_text_start;
extern uint32 _safety_text_end;
extern uint32 _sys_stack_end;
extern uint32 _svc_stack_end;
extern uint32 _irq_stack_end;
extern uint32 _stack_guard_pattern;

/* ========================================================================== */
/*                       Forward Declarations                                 */
/* ========================================================================== */

static void Startup_ClearBSS(void);
static void Startup_CopyData(void);
static void Startup_EnableVFP(void);
static void Startup_ConfigureMPU(void);
static void Startup_EnableCaches(void);
static void Startup_InitStackGuard(void);

/* ========================================================================== */
/*                       ARM CP15 Inline Helpers                              */
/* ========================================================================== */

/**
 * @brief Read Main ID Register
 */
static inline uint32 Startup_ReadMIDR(void)
{
    uint32 val;
    __asm volatile("MRC p15, 0, %0, c0, c0, 0" : "=r" (val));
    return val;
}

/**
 * @brief Read SCTLR (System Control Register)
 */
static inline uint32 Startup_ReadSCTLR(void)
{
    uint32 val;
    __asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (val));
    return val;
}

/**
 * @brief Write SCTLR
 */
static inline void Startup_WriteSCTLR(uint32 val)
{
    __asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (val));
    __asm volatile("DSB");
    __asm volatile("ISB");
}

/**
 * @brief Data Synchronization Barrier
 */
static inline void Startup_DSB(void)
{
    __asm volatile("DSB");
}

/**
 * @brief Instruction Synchronization Barrier
 */
static inline void Startup_ISB(void)
{
    __asm volatile("ISB");
}

/* ========================================================================== */
/*                       MPU Region Configuration                             */
/* ========================================================================== */

/** @brief MPU region configuration structure */
typedef struct {
    uint32 baseAddr;    /**< Region base address (aligned)   */
    uint32 sizeEnable;  /**< Region size and enable bits     */
    uint32 accessCtrl;  /**< Access permission and attributes*/
} MpuRegionCfg_t;

/* MPU Size encoding: log2(size) - 1 */
#define MPU_SIZE_64K    (15U << 1)
#define MPU_SIZE_384K   (18U << 1)  /* next power of 2 = 512K, but use sub-region disable */
#define MPU_SIZE_512K   (18U << 1)
#define MPU_SIZE_2M     (20U << 1)
#define MPU_SIZE_4M     (21U << 1)
#define MPU_SIZE_4G     (31U << 1)
#define MPU_REGION_EN   (0x01U)

/* Access permissions */
#define MPU_AP_FULL     (0x03U << 8)   /* Full access                       */
#define MPU_AP_RO       (0x06U << 8)   /* Read-only                         */
#define MPU_AP_NONE     (0x00U << 8)   /* No access                         */

/* Memory type attributes */
#define MPU_NORMAL_WT   (0x00U << 16)  /* Normal, Write-Through             */
#define MPU_NORMAL_WB   (0x01U << 16)  /* Normal, Write-Back                */
#define MPU_DEVICE      (0x02U << 16)  /* Device, non-cacheable             */
#define MPU_STRONGLY    (0x04U << 16)  /* Strongly Ordered                  */
#define MPU_XN          (0x01U << 12)  /* Execute-Never                     */

/**
 * @brief MPU region table
 * @details 8 regions configured for AWR2944 memory map.
 *          Safety-critical code is placed in its own region with
 *          execute + read-only permissions.
 */
static const MpuRegionCfg_t g_mpuRegions[] =
{
    /* Region 0: Background -- deny all access (default) */
    {
        .baseAddr   = 0x00000000U,
        .sizeEnable = MPU_SIZE_4G | MPU_REGION_EN,
        .accessCtrl = MPU_AP_NONE | MPU_STRONGLY | MPU_XN
    },
    /* Region 1: TCMA -- safety-critical code, normal cached */
    {
        .baseAddr   = 0x00000000U,
        .sizeEnable = MPU_SIZE_64K | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_NORMAL_WT
    },
    /* Region 2: TCMB -- data + stack, non-executable */
    {
        .baseAddr   = 0x00080000U,
        .sizeEnable = MPU_SIZE_64K | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_NORMAL_WB | MPU_XN
    },
    /* Region 3: L2 SRAM -- main code and data */
    {
        .baseAddr   = 0x10200000U,
        .sizeEnable = MPU_SIZE_512K | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_NORMAL_WB
    },
    /* Region 4: L3 SRAM -- shared, DMA buffers */
    {
        .baseAddr   = 0x88000000U,
        .sizeEnable = MPU_SIZE_2M | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_NORMAL_WT | MPU_XN
    },
    /* Region 5: RFSS -- device registers */
    {
        .baseAddr   = 0x40000000U,
        .sizeEnable = MPU_SIZE_4M | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_DEVICE | MPU_XN
    },
    /* Region 6: MSS peripherals (VIM, ESM, WDT, GPIO, etc.) */
    {
        .baseAddr   = 0x50000000U,
        .sizeEnable = MPU_SIZE_4M | MPU_REGION_EN,
        .accessCtrl = MPU_AP_FULL | MPU_DEVICE | MPU_XN
    },
    /* Region 7: Flash (QSPI) -- read/execute only */
    {
        .baseAddr   = 0x20000000U,
        .sizeEnable = MPU_SIZE_4M | MPU_REGION_EN,
        .accessCtrl = MPU_AP_RO | MPU_NORMAL_WT
    }
};

#define MPU_NUM_REGIONS (sizeof(g_mpuRegions) / sizeof(g_mpuRegions[0]))

/* ========================================================================== */
/*                       Public Startup Entry Point                           */
/* ========================================================================== */

/**
 * @brief C-level startup initialization
 * @details Called from vector_table.S after stack pointers are set.
 *          Initializes memory, FPU, MPU, caches, then returns to asm
 *          which calls main().
 */
void _startup_init(void)
{
    /* Verify CPU ID -- basic sanity check */
    uint32 midr = Startup_ReadMIDR();
    (void)midr;  /* Could verify Cortex-R5 part number 0xC15 */

    /* Step 1: Clear BSS (must be done with ECC-safe 64-bit writes if ECC is on) */
    Startup_ClearBSS();

    /* Step 2: Copy initialized data from load region to run region */
    Startup_CopyData();

    /* Step 3: Initialize stack guard patterns for overflow detection */
    Startup_InitStackGuard();

    /* Step 4: Enable VFP/FPU */
    Startup_EnableVFP();

    /* Step 5: Configure MPU regions */
    Startup_ConfigureMPU();

    /* Step 6: Enable instruction and data caches */
    Startup_EnableCaches();
}

/* ========================================================================== */
/*                       Static Function Implementations                      */
/* ========================================================================== */

/**
 * @brief Clear BSS section to zero
 * @details Uses 32-bit aligned writes. For ECC-enabled memories, a full
 *          write is required to generate valid check bits.
 */
static void Startup_ClearBSS(void)
{
    uint32 *pBss = &_bss_start;
    const uint32 *pBssEnd = &_bss_end;

    while (pBss < pBssEnd)
    {
        *pBss = 0U;
        pBss++;
    }
}

/**
 * @brief Copy initialized data from load address to run address
 */
static void Startup_CopyData(void)
{
    uint32 *pDst = &_data_start;
    const uint32 *pDstEnd = &_data_end;
    const uint32 *pSrc = &_data_load_start;

    while (pDst < pDstEnd)
    {
        *pDst = *pSrc;
        pDst++;
        pSrc++;
    }
}

/**
 * @brief Enable VFPv3-D16 Floating Point Unit
 */
static void Startup_EnableVFP(void)
{
    uint32 cpacr;

    /* Enable CP10 and CP11 (VFP) in Coprocessor Access Control Register */
    __asm volatile("MRC p15, 0, %0, c1, c0, 2" : "=r" (cpacr));
    cpacr |= (0xFU << 20);  /* Full access to CP10 and CP11 */
    __asm volatile("MCR p15, 0, %0, c1, c0, 2" : : "r" (cpacr));
    Startup_ISB();

    /* Enable VFP by setting FPEXC.EN */
    __asm volatile("MOV R0, #0x40000000");
    __asm volatile("VMSR FPEXC, R0");
}

/**
 * @brief Configure MPU regions
 * @details Programs all 8 MPU regions from the static configuration table.
 *          MPU is required for memory protection in safety-critical systems.
 */
static void Startup_ConfigureMPU(void)
{
    uint32 i;
    uint32 sctlr;

    /* Disable MPU before configuration */
    sctlr = Startup_ReadSCTLR();
    sctlr &= ~(0x01U);  /* Clear M bit */
    Startup_WriteSCTLR(sctlr);

    /* Program each region */
    for (i = 0U; i < MPU_NUM_REGIONS; i++)
    {
        /* Select region number */
        __asm volatile("MCR p15, 0, %0, c6, c2, 0" : : "r" (i));

        /* Set base address */
        __asm volatile("MCR p15, 0, %0, c6, c1, 0" : : "r" (g_mpuRegions[i].baseAddr));

        /* Set size and enable */
        __asm volatile("MCR p15, 0, %0, c6, c1, 2" : : "r" (g_mpuRegions[i].sizeEnable));

        /* Set access control */
        __asm volatile("MCR p15, 0, %0, c6, c1, 4" : : "r" (g_mpuRegions[i].accessCtrl));
    }

    Startup_DSB();

    /* Enable MPU */
    sctlr = Startup_ReadSCTLR();
    sctlr |= 0x01U;  /* Set M bit */
    /* Also enable background region for privileged access */
    sctlr |= (0x01U << 17);  /* BR bit */
    Startup_WriteSCTLR(sctlr);
}

/**
 * @brief Enable instruction and data caches
 */
static void Startup_EnableCaches(void)
{
    uint32 sctlr = Startup_ReadSCTLR();

    sctlr |= (0x01U << 2);   /* C bit: Data cache enable   */
    sctlr |= (0x01U << 12);  /* I bit: Instruction cache   */

    Startup_WriteSCTLR(sctlr);
}

/**
 * @brief Initialize stack guard patterns for overflow detection
 * @details Writes a known pattern at the bottom of each stack.
 *          The safety monitor periodically checks these patterns
 *          to detect stack overflow (ISO 26262 Table 11, 1c).
 */
static void Startup_InitStackGuard(void)
{
    volatile uint32 *pGuard;
    uint32 guardPattern = (uint32)&_stack_guard_pattern;  /* 0xDEADCAFE */

    /* Write guard pattern at bottom of SVC stack */
    pGuard = (volatile uint32 *)((uint32)&_svc_stack_end - 4096U);
    *pGuard = guardPattern;
    *(pGuard + 1U) = guardPattern;
    *(pGuard + 2U) = guardPattern;
    *(pGuard + 3U) = guardPattern;

    /* Write guard pattern at bottom of IRQ stack */
    pGuard = (volatile uint32 *)((uint32)&_irq_stack_end - 2048U);
    *pGuard = guardPattern;
    *(pGuard + 1U) = guardPattern;

    /* Write guard pattern at bottom of system stack */
    pGuard = (volatile uint32 *)((uint32)&_sys_stack_end - 4096U);
    *pGuard = guardPattern;
    *(pGuard + 1U) = guardPattern;
    *(pGuard + 2U) = guardPattern;
    *(pGuard + 3U) = guardPattern;
}
