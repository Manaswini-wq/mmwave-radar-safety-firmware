/**
 * @file awr2944_regs.h
 * @brief AWR2944 Register Definitions for bare-metal firmware
 * @details Memory-mapped register base addresses and offsets for the
 *          AWR2944 mixed-signal analog and digital SOC. Covers MSS (R5F),
 *          RF/Analog subsystem, DSS, and peripheral modules.
 *
 * @note Addresses derived from TI AWR2944 Technical Reference Manual (SWRU583)
 *
 * @copyright Copyright (c) 2024
 * @license MIT
 */
#ifndef AWR2944_REGS_H
#define AWR2944_REGS_H

#include "platform_types.h"

/* ========================================================================== */
/*                      Register Access Macros                                */
/* ========================================================================== */

/** @brief Read a 32-bit memory-mapped register */
#define REG32_RD(addr)        (*((volatile uint32 *)(addr)))

/** @brief Write a 32-bit memory-mapped register */
#define REG32_WR(addr, val)   (*((volatile uint32 *)(addr)) = (uint32)(val))

/** @brief Set bits in a register */
#define REG32_SET(addr, mask) (REG32_WR((addr), REG32_RD((addr)) | (uint32)(mask)))

/** @brief Clear bits in a register */
#define REG32_CLR(addr, mask) (REG32_WR((addr), REG32_RD((addr)) & ~(uint32)(mask)))

/** @brief Read-Modify-Write a register field */
#define REG32_RMW(addr, mask, shift, val) \
    REG32_WR((addr), (REG32_RD((addr)) & ~(uint32)(mask)) | \
             (((uint32)(val) << (shift)) & (uint32)(mask)))

/** @brief Extract a register field */
#define REG32_FIELD_GET(addr, mask, shift) \
    (((REG32_RD(addr)) & (uint32)(mask)) >> (shift))

/* ========================================================================== */
/*              MSS (Master Subsystem) -- R5F Core                            */
/* ========================================================================== */

/** @defgroup MSS_BASE MSS Base Addresses
 *  @{
 */
#define MSS_BASE_ADDR                   (0x02000000U)
#define MSS_TOPRCM_BASE                 (0x02100000U)
#define MSS_RCM_BASE                    (0x02110000U)
#define MSS_CTRL_BASE                   (0x02120000U)
#define MSS_DEBUGSS_BASE                (0x02130000U)

#define MSS_TCMA_BASE                   (0x00000000U)  /**< R5F TCMA (64KB)         */
#define MSS_TCMB_BASE                   (0x00080000U)  /**< R5F TCMB (64KB)         */
#define MSS_L2RAM_BASE                  (0x10200000U)  /**< L2 SRAM (384KB)         */
#define MSS_L3RAM_BASE                  (0x88000000U)  /**< L3 SRAM (2MB shared)    */
#define MSS_FLASH_BASE                  (0x20000000U)  /**< QSPI Flash              */
/** @} */

/* ---- MSS CTRL Registers ---- */
#define MSS_CTRL_R5_STATUS              (MSS_CTRL_BASE + 0x0000U)
#define MSS_CTRL_R5_CONTROL             (MSS_CTRL_BASE + 0x0004U)
#define MSS_CTRL_R5_INIT                (MSS_CTRL_BASE + 0x0008U)
#define MSS_CTRL_R5_COREA_HALT          (MSS_CTRL_BASE + 0x000CU)
#define MSS_CTRL_R5_COREB_HALT          (MSS_CTRL_BASE + 0x0010U)
#define MSS_CTRL_R5_LOCKSTEP_EN         (MSS_CTRL_BASE + 0x0014U)
#define MSS_CTRL_R5_LOCK_OVERRIDE       (MSS_CTRL_BASE + 0x0018U)

/* Lockstep bits */
#define LOCKSTEP_EN_MASK                (0x00000007U)
#define LOCKSTEP_EN_ENABLE              (0x00000007U)
#define LOCKSTEP_EN_DISABLE             (0x00000000U)

/* ========================================================================== */
/*              VIM (Vectored Interrupt Manager)                              */
/* ========================================================================== */

#define MSS_VIM_BASE                    (0x50F00000U)
#define VIM_PID                         (MSS_VIM_BASE + 0x0000U)
#define VIM_INFO                        (MSS_VIM_BASE + 0x0004U)
#define VIM_PRIIRQ                      (MSS_VIM_BASE + 0x0008U)
#define VIM_PRIFIQ                      (MSS_VIM_BASE + 0x000CU)
#define VIM_IRQGSTS                     (MSS_VIM_BASE + 0x0010U)
#define VIM_FIQGSTS                     (MSS_VIM_BASE + 0x0014U)
#define VIM_IRQVEC                      (MSS_VIM_BASE + 0x0018U)
#define VIM_FIQVEC                      (MSS_VIM_BASE + 0x001CU)
#define VIM_ACTIRQ                      (MSS_VIM_BASE + 0x0020U)
#define VIM_ACTFIQ                      (MSS_VIM_BASE + 0x0024U)
#define VIM_DEDVEC                      (MSS_VIM_BASE + 0x0028U)

/* VIM interrupt enable/status for groups of 32 */
#define VIM_INTR_EN_SET(n)              (MSS_VIM_BASE + 0x0400U + ((n) * 0x20U))
#define VIM_INTR_EN_CLR(n)              (MSS_VIM_BASE + 0x0404U + ((n) * 0x20U))
#define VIM_STS(n)                      (MSS_VIM_BASE + 0x0408U + ((n) * 0x20U))
#define VIM_RAW_STS(n)                  (MSS_VIM_BASE + 0x040CU + ((n) * 0x20U))

/* VIM priority and vector registers per interrupt */
#define VIM_PRI(n)                      (MSS_VIM_BASE + 0x1000U + ((n) * 4U))
#define VIM_VEC(n)                      (MSS_VIM_BASE + 0x2000U + ((n) * 4U))

#define VIM_MAX_INTERRUPTS              256U

/* ========================================================================== */
/*              ESM (Error Signaling Module)                                  */
/* ========================================================================== */

#define MSS_ESM_BASE                    (0x50D00000U)
#define ESM_PID                         (MSS_ESM_BASE + 0x0000U)
#define ESM_INFO                        (MSS_ESM_BASE + 0x0004U)
#define ESM_EN                          (MSS_ESM_BASE + 0x0008U)
#define ESM_SFT_RST                     (MSS_ESM_BASE + 0x000CU)

/* ESM Group 1 (low priority, maskable) */
#define ESM_G1_RAW(n)                   (MSS_ESM_BASE + 0x0040U + ((n) * 4U))
#define ESM_G1_STS(n)                   (MSS_ESM_BASE + 0x0080U + ((n) * 4U))
#define ESM_G1_EN_SET(n)                (MSS_ESM_BASE + 0x0100U + ((n) * 4U))
#define ESM_G1_EN_CLR(n)                (MSS_ESM_BASE + 0x0180U + ((n) * 4U))

/* ESM Group 2 (high priority, NMI) */
#define ESM_G2_RAW(n)                   (MSS_ESM_BASE + 0x0200U + ((n) * 4U))
#define ESM_G2_STS(n)                   (MSS_ESM_BASE + 0x0240U + ((n) * 4U))
#define ESM_G2_EN_SET(n)                (MSS_ESM_BASE + 0x0280U + ((n) * 4U))
#define ESM_G2_EN_CLR(n)                (MSS_ESM_BASE + 0x02C0U + ((n) * 4U))

/* ESM Group 3 (direct pin output) */
#define ESM_G3_RAW(n)                   (MSS_ESM_BASE + 0x0300U + ((n) * 4U))
#define ESM_G3_STS(n)                   (MSS_ESM_BASE + 0x0340U + ((n) * 4U))
#define ESM_G3_EN_SET(n)                (MSS_ESM_BASE + 0x0380U + ((n) * 4U))

/* ESM error codes */
#define ESM_ERR_LOCKSTEP_COMPARE        (0U)
#define ESM_ERR_ECC_TCMA_SEC            (1U)
#define ESM_ERR_ECC_TCMA_DED            (2U)
#define ESM_ERR_ECC_TCMB_SEC            (3U)
#define ESM_ERR_ECC_TCMB_DED            (4U)
#define ESM_ERR_ECC_L2_SEC              (5U)
#define ESM_ERR_ECC_L2_DED              (6U)
#define ESM_ERR_WDT_NMI                 (7U)
#define ESM_ERR_PLL_SLIP                (8U)
#define ESM_ERR_VOLTAGE_MON             (9U)
#define ESM_ERR_TEMP_MON                (10U)
#define ESM_ERR_DMA_MPU                 (11U)
#define ESM_ERR_VIM_RAM_PARITY          (12U)
#define ESM_ERR_RF_POWER_FAULT          (13U)

/* ========================================================================== */
/*              RTI / Watchdog Timer                                          */
/* ========================================================================== */

#define MSS_WDT_BASE                    (0x52180000U)
#define WDT_RTIDWDCTRL                  (MSS_WDT_BASE + 0x0090U)
#define WDT_RTIDWDPRLD                  (MSS_WDT_BASE + 0x0094U)
#define WDT_RTIWDSTATUS                 (MSS_WDT_BASE + 0x0098U)
#define WDT_RTIWDKEY                    (MSS_WDT_BASE + 0x009CU)
#define WDT_RTIDWDCNTR                  (MSS_WDT_BASE + 0x00A0U)
#define WDT_RTIWWDRXNCTRL              (MSS_WDT_BASE + 0x00A4U)
#define WDT_RTIWWDSIZECTRL             (MSS_WDT_BASE + 0x00A8U)

/* Watchdog control bits */
#define WDT_CTRL_ENABLE                 (0xA98559DAU)
#define WDT_CTRL_DISABLE                (0x00000000U)
#define WDT_KEY_FIRST                   (0x0000E51AU)
#define WDT_KEY_SECOND                  (0x0000A35CU)
#define WDT_STATUS_VIOLATION            (0x00000004U)
#define WDT_STATUS_KEY_SEQ_ERR          (0x00000002U)
#define WDT_STATUS_END_TIME_VIOL        (0x00000001U)

/* Window size selections */
#define WDT_WINDOW_100_PERCENT          (0x00000005U)
#define WDT_WINDOW_50_PERCENT           (0x00000050U)
#define WDT_WINDOW_25_PERCENT           (0x00000500U)
#define WDT_WINDOW_12_5_PERCENT         (0x00005000U)

/* ========================================================================== */
/*              ECC Aggregator                                                */
/* ========================================================================== */

#define MSS_ECC_AGG_BASE                (0x53000000U)
#define ECC_AGG_REVISION                (MSS_ECC_AGG_BASE + 0x0000U)
#define ECC_AGG_CTRL                    (MSS_ECC_AGG_BASE + 0x0004U)
#define ECC_AGG_ERR_CTRL1               (MSS_ECC_AGG_BASE + 0x0008U)
#define ECC_AGG_ERR_CTRL2               (MSS_ECC_AGG_BASE + 0x000CU)
#define ECC_AGG_ERR_STATUS1             (MSS_ECC_AGG_BASE + 0x0010U)
#define ECC_AGG_ERR_STATUS2             (MSS_ECC_AGG_BASE + 0x0014U)
#define ECC_AGG_ERR_STATUS3             (MSS_ECC_AGG_BASE + 0x0018U)
#define ECC_AGG_SEC_EOI                 (MSS_ECC_AGG_BASE + 0x003CU)
#define ECC_AGG_SEC_STATUS(n)           (MSS_ECC_AGG_BASE + 0x0040U + ((n) * 4U))
#define ECC_AGG_DED_EOI                 (MSS_ECC_AGG_BASE + 0x007CU)
#define ECC_AGG_DED_STATUS(n)           (MSS_ECC_AGG_BASE + 0x0080U + ((n) * 4U))

/* ECC control bits */
#define ECC_CTRL_ENABLE                 (0x00000001U)
#define ECC_CTRL_CHECK_ENABLE           (0x00000002U)
#define ECC_CTRL_ERR_INJECT             (0x00000004U)
#define ECC_SEC_ERROR_BIT               (0x00000001U)
#define ECC_DED_ERROR_BIT               (0x00000001U)

/* ========================================================================== */
/*              RF/Analog Subsystem (RFSS)                                    */
/* ========================================================================== */

/** @defgroup RFSS RF Subsystem Registers
 *  @{
 */
#define RFSS_BASE                       (0x40000000U)
#define RFSS_CTRL_BASE                  (RFSS_BASE + 0x00000000U)
#define RFSS_CHIRP_BASE                 (RFSS_BASE + 0x00010000U)
#define RFSS_TX_BASE                    (RFSS_BASE + 0x00020000U)
#define RFSS_RX_BASE                    (RFSS_BASE + 0x00030000U)
#define RFSS_PLL_BASE                   (RFSS_BASE + 0x00040000U)
#define RFSS_ADC_BASE                   (RFSS_BASE + 0x00050000U)
#define RFSS_DAC_BASE                   (RFSS_BASE + 0x00060000U)
#define RFSS_LO_BASE                    (RFSS_BASE + 0x00070000U)
/** @} */

/* ---- RFSS Control Registers ---- */
#define RFSS_CTRL_PWR_ON                (RFSS_CTRL_BASE + 0x0000U)
#define RFSS_CTRL_CLK_EN                (RFSS_CTRL_BASE + 0x0004U)
#define RFSS_CTRL_RESET                 (RFSS_CTRL_BASE + 0x0008U)
#define RFSS_CTRL_STATUS                (RFSS_CTRL_BASE + 0x000CU)
#define RFSS_CTRL_RF_ENABLE             (RFSS_CTRL_BASE + 0x0010U)

/* ---- Chirp Engine Registers ---- */
#define CHIRP_CTRL                      (RFSS_CHIRP_BASE + 0x0000U)
#define CHIRP_STATUS                    (RFSS_CHIRP_BASE + 0x0004U)
#define CHIRP_START_FREQ_L              (RFSS_CHIRP_BASE + 0x0008U)
#define CHIRP_START_FREQ_H              (RFSS_CHIRP_BASE + 0x000CU)
#define CHIRP_FREQ_SLOPE_L              (RFSS_CHIRP_BASE + 0x0010U)
#define CHIRP_FREQ_SLOPE_H              (RFSS_CHIRP_BASE + 0x0014U)
#define CHIRP_IDLE_TIME                 (RFSS_CHIRP_BASE + 0x0018U)
#define CHIRP_ADC_START_TIME            (RFSS_CHIRP_BASE + 0x001CU)
#define CHIRP_NUM_ADC_SAMPLES           (RFSS_CHIRP_BASE + 0x0020U)
#define CHIRP_PROFILE_IDX               (RFSS_CHIRP_BASE + 0x0024U)
#define CHIRP_TX_EN_MASK                (RFSS_CHIRP_BASE + 0x0028U)
#define CHIRP_RX_EN_MASK                (RFSS_CHIRP_BASE + 0x002CU)
#define CHIRP_NUM_LOOPS                 (RFSS_CHIRP_BASE + 0x0030U)
#define CHIRP_BURST_PERIOD              (RFSS_CHIRP_BASE + 0x0034U)
#define CHIRP_FRAME_PERIOD              (RFSS_CHIRP_BASE + 0x0038U)
#define CHIRP_NUM_FRAMES                (RFSS_CHIRP_BASE + 0x003CU)
#define CHIRP_SEQ_TRIGGER               (RFSS_CHIRP_BASE + 0x0040U)
#define CHIRP_SEQ_FORCE_STOP            (RFSS_CHIRP_BASE + 0x0044U)

/* Chirp control bits */
#define CHIRP_CTRL_ENABLE               (0x00000001U)
#define CHIRP_CTRL_SW_TRIGGER           (0x00000002U)
#define CHIRP_CTRL_CONT_MODE            (0x00000004U)
#define CHIRP_STATUS_ACTIVE             (0x00000001U)
#define CHIRP_STATUS_FRAME_DONE         (0x00000002U)
#define CHIRP_STATUS_CHIRP_DONE         (0x00000004U)
#define CHIRP_STATUS_ERROR              (0x00000080U)

/* ---- TX Channel Registers ---- */
#define TX_CH_CTRL(ch)                  (RFSS_TX_BASE + 0x0000U + ((ch) * 0x40U))
#define TX_CH_POWER(ch)                 (RFSS_TX_BASE + 0x0004U + ((ch) * 0x40U))
#define TX_CH_PHASE(ch)                 (RFSS_TX_BASE + 0x0008U + ((ch) * 0x40U))
#define TX_CH_STATUS(ch)                (RFSS_TX_BASE + 0x000CU + ((ch) * 0x40U))
#define TX_CH_LOOPBACK(ch)              (RFSS_TX_BASE + 0x0010U + ((ch) * 0x40U))
#define TX_CH_CAL_OFFSET(ch)            (RFSS_TX_BASE + 0x0014U + ((ch) * 0x40U))
#define TX_CH_EN_MASK                   (0x00000001U)
#define TX_CH_POWER_MAX                 (0x0000001FU)

/* ---- RX Channel Registers ---- */
#define RX_CH_CTRL(ch)                  (RFSS_RX_BASE + 0x0000U + ((ch) * 0x40U))
#define RX_CH_GAIN(ch)                  (RFSS_RX_BASE + 0x0004U + ((ch) * 0x40U))
#define RX_CH_PHASE(ch)                 (RFSS_RX_BASE + 0x0008U + ((ch) * 0x40U))
#define RX_CH_STATUS(ch)                (RFSS_RX_BASE + 0x000CU + ((ch) * 0x40U))
#define RX_CH_IF_FILTER(ch)             (RFSS_RX_BASE + 0x0010U + ((ch) * 0x40U))
#define RX_CH_HPF_CTRL(ch)              (RFSS_RX_BASE + 0x0014U + ((ch) * 0x40U))
#define RX_CH_DC_OFFSET_I(ch)           (RFSS_RX_BASE + 0x0018U + ((ch) * 0x40U))
#define RX_CH_DC_OFFSET_Q(ch)           (RFSS_RX_BASE + 0x001CU + ((ch) * 0x40U))
#define RX_CH_CAL_GAIN(ch)              (RFSS_RX_BASE + 0x0020U + ((ch) * 0x40U))
#define RX_CH_CAL_PHASE(ch)             (RFSS_RX_BASE + 0x0024U + ((ch) * 0x40U))
#define RX_CH_EN_MASK                   (0x00000001U)

/* ---- PLL / Synthesizer Registers ---- */
#define PLL_CTRL                        (RFSS_PLL_BASE + 0x0000U)
#define PLL_STATUS                      (RFSS_PLL_BASE + 0x0004U)
#define PLL_FREQ_WORD_L                 (RFSS_PLL_BASE + 0x0008U)
#define PLL_FREQ_WORD_H                 (RFSS_PLL_BASE + 0x000CU)
#define PLL_LOOP_BW                     (RFSS_PLL_BASE + 0x0010U)
#define PLL_CHARGE_PUMP                 (RFSS_PLL_BASE + 0x0014U)
#define PLL_VCO_CTRL                    (RFSS_PLL_BASE + 0x0018U)
#define PLL_DIVIDER_N                   (RFSS_PLL_BASE + 0x001CU)
#define PLL_DIVIDER_R                   (RFSS_PLL_BASE + 0x0020U)
#define PLL_CAL_CTRL                    (RFSS_PLL_BASE + 0x0024U)
#define PLL_CAL_STATUS                  (RFSS_PLL_BASE + 0x0028U)

#define PLL_CTRL_ENABLE                 (0x00000001U)
#define PLL_STATUS_LOCKED               (0x00000001U)
#define PLL_STATUS_SLIP                 (0x00000002U)
#define PLL_STATUS_CAL_DONE             (0x00000004U)

/* ---- ADC Registers ---- */
#define ADC_CTRL                        (RFSS_ADC_BASE + 0x0000U)
#define ADC_STATUS                      (RFSS_ADC_BASE + 0x0004U)
#define ADC_CLK_DIV                     (RFSS_ADC_BASE + 0x0008U)
#define ADC_RESOLUTION                  (RFSS_ADC_BASE + 0x000CU)
#define ADC_SAMPLE_RATE                 (RFSS_ADC_BASE + 0x0010U)
#define ADC_NUM_SAMPLES                 (RFSS_ADC_BASE + 0x0014U)
#define ADC_DATA_FORMAT                 (RFSS_ADC_BASE + 0x0018U)
#define ADC_LOOPBACK_CTRL               (RFSS_ADC_BASE + 0x001CU)
#define ADC_TEST_PATTERN                (RFSS_ADC_BASE + 0x0020U)
#define ADC_INT_ENABLE                  (RFSS_ADC_BASE + 0x0024U)
#define ADC_INT_STATUS                  (RFSS_ADC_BASE + 0x0028U)
#define ADC_INT_CLEAR                   (RFSS_ADC_BASE + 0x002CU)
#define ADC_DMA_TRIGGER                 (RFSS_ADC_BASE + 0x0030U)
#define ADC_BUFFER_BASE                 (RFSS_ADC_BASE + 0x1000U)

#define ADC_CTRL_ENABLE                 (0x00000001U)
#define ADC_CTRL_RESET                  (0x00000002U)
#define ADC_CTRL_LOOPBACK               (0x00000010U)
#define ADC_RES_12BIT                   (0x00000000U)
#define ADC_RES_14BIT                   (0x00000001U)
#define ADC_RES_16BIT                   (0x00000002U)
#define ADC_FORMAT_REAL                 (0x00000000U)
#define ADC_FORMAT_COMPLEX              (0x00000001U)
#define ADC_INT_BUFFER_FULL             (0x00000001U)
#define ADC_INT_OVERFLOW                (0x00000002U)

/* ---- DAC Registers ---- */
#define DAC_CTRL                        (RFSS_DAC_BASE + 0x0000U)
#define DAC_STATUS                      (RFSS_DAC_BASE + 0x0004U)
#define DAC_DATA                        (RFSS_DAC_BASE + 0x0008U)
#define DAC_CLK_DIV                     (RFSS_DAC_BASE + 0x000CU)
#define DAC_GAIN                        (RFSS_DAC_BASE + 0x0010U)
#define DAC_OFFSET                      (RFSS_DAC_BASE + 0x0014U)
#define DAC_CAL_CTRL                    (RFSS_DAC_BASE + 0x0018U)

#define DAC_CTRL_ENABLE                 (0x00000001U)
#define DAC_CTRL_RESET                  (0x00000002U)

/* ---- LO Leakage Registers ---- */
#define LO_CTRL                         (RFSS_LO_BASE + 0x0000U)
#define LO_DC_OFFSET_I                  (RFSS_LO_BASE + 0x0004U)
#define LO_DC_OFFSET_Q                  (RFSS_LO_BASE + 0x0008U)
#define LO_NULL_CTRL                    (RFSS_LO_BASE + 0x000CU)
#define LO_NULL_STATUS                  (RFSS_LO_BASE + 0x0010U)
#define LO_RESIDUAL_I                   (RFSS_LO_BASE + 0x0014U)
#define LO_RESIDUAL_Q                   (RFSS_LO_BASE + 0x0018U)

/* ========================================================================== */
/*              DSS (DSP Subsystem)                                           */
/* ========================================================================== */

#define DSS_BASE                        (0x06000000U)
#define DSS_CTRL_BASE                   (DSS_BASE + 0x00000000U)
#define DSS_DMA_BASE                    (DSS_BASE + 0x00010000U)
#define DSS_DATA_MEM_BASE               (DSS_BASE + 0x00100000U)

/* ---- DMA Registers ---- */
#define DMA_CTRL                        (DSS_DMA_BASE + 0x0000U)
#define DMA_STATUS                      (DSS_DMA_BASE + 0x0004U)
#define DMA_INT_EN                      (DSS_DMA_BASE + 0x0008U)
#define DMA_INT_STATUS                  (DSS_DMA_BASE + 0x000CU)
#define DMA_INT_CLEAR                   (DSS_DMA_BASE + 0x0010U)

/* Per-channel DMA registers (16 channels) */
#define DMA_CH_CTRL(ch)                 (DSS_DMA_BASE + 0x0100U + ((ch) * 0x20U))
#define DMA_CH_SRC_ADDR(ch)             (DSS_DMA_BASE + 0x0104U + ((ch) * 0x20U))
#define DMA_CH_DST_ADDR(ch)             (DSS_DMA_BASE + 0x0108U + ((ch) * 0x20U))
#define DMA_CH_XFER_SIZE(ch)            (DSS_DMA_BASE + 0x010CU + ((ch) * 0x20U))
#define DMA_CH_STATUS(ch)               (DSS_DMA_BASE + 0x0110U + ((ch) * 0x20U))
#define DMA_CH_LINK(ch)                 (DSS_DMA_BASE + 0x0114U + ((ch) * 0x20U))

#define DMA_CH_CTRL_ENABLE              (0x00000001U)
#define DMA_CH_CTRL_TRIGGER             (0x00000002U)
#define DMA_CH_CTRL_CHAIN_EN            (0x00000010U)
#define DMA_CH_CTRL_INT_EN              (0x00000100U)
#define DMA_MAX_CHANNELS                16U

/* ========================================================================== */
/*              GPIO                                                          */
/* ========================================================================== */

#define MSS_GPIO_BASE                   (0x02140000U)
#define GPIO_DIR(bank)                  (MSS_GPIO_BASE + 0x0000U + ((bank) * 0x40U))
#define GPIO_OUT(bank)                  (MSS_GPIO_BASE + 0x0004U + ((bank) * 0x40U))
#define GPIO_IN(bank)                   (MSS_GPIO_BASE + 0x0008U + ((bank) * 0x40U))
#define GPIO_SET(bank)                  (MSS_GPIO_BASE + 0x000CU + ((bank) * 0x40U))
#define GPIO_CLR(bank)                  (MSS_GPIO_BASE + 0x0010U + ((bank) * 0x40U))
#define GPIO_TOGGLE(bank)               (MSS_GPIO_BASE + 0x0014U + ((bank) * 0x40U))
#define GPIO_INT_EN(bank)               (MSS_GPIO_BASE + 0x0018U + ((bank) * 0x40U))
#define GPIO_INT_STATUS(bank)           (MSS_GPIO_BASE + 0x001CU + ((bank) * 0x40U))
#define GPIO_MUX_CTRL(pin)              (MSS_GPIO_BASE + 0x0200U + ((pin) * 4U))

#define GPIO_MAX_PINS                   64U
#define GPIO_BANK_SIZE                  32U

/* ========================================================================== */
/*              SPI (Serial Peripheral Interface)                             */
/* ========================================================================== */

#define MSS_SPI_BASE                    (0x02160000U)
#define SPI_CTRL0                       (MSS_SPI_BASE + 0x0000U)
#define SPI_CTRL1                       (MSS_SPI_BASE + 0x0004U)
#define SPI_STATUS                      (MSS_SPI_BASE + 0x0008U)
#define SPI_DATA                        (MSS_SPI_BASE + 0x000CU)
#define SPI_CLK_DIV                     (MSS_SPI_BASE + 0x0010U)
#define SPI_CS_CTRL                     (MSS_SPI_BASE + 0x0014U)
#define SPI_INT_EN                      (MSS_SPI_BASE + 0x0018U)
#define SPI_INT_STATUS                  (MSS_SPI_BASE + 0x001CU)
#define SPI_INT_CLEAR                   (MSS_SPI_BASE + 0x0020U)
#define SPI_TX_FIFO                     (MSS_SPI_BASE + 0x0100U)
#define SPI_RX_FIFO                     (MSS_SPI_BASE + 0x0200U)

#define SPI_CTRL_ENABLE                 (0x00000001U)
#define SPI_CTRL_MASTER                 (0x00000002U)
#define SPI_CTRL_CPOL                   (0x00000004U)
#define SPI_CTRL_CPHA                   (0x00000008U)
#define SPI_STATUS_BUSY                 (0x00000001U)
#define SPI_STATUS_TX_EMPTY             (0x00000002U)
#define SPI_STATUS_RX_FULL              (0x00000004U)
#define SPI_INT_TX_COMPLETE             (0x00000001U)
#define SPI_INT_RX_COMPLETE             (0x00000002U)

/* ========================================================================== */
/*              Timer (RTI)                                                   */
/* ========================================================================== */

#define MSS_TIMER_BASE                  (0x52180000U)
#define TIMER_GCTRL                     (MSS_TIMER_BASE + 0x0000U)
#define TIMER_TBCTRL                    (MSS_TIMER_BASE + 0x0004U)
#define TIMER_CAPCTRL                   (MSS_TIMER_BASE + 0x0008U)
#define TIMER_COMPCTRL                  (MSS_TIMER_BASE + 0x000CU)

/* Counter 0 */
#define TIMER_FRC0                      (MSS_TIMER_BASE + 0x0010U)
#define TIMER_UC0                       (MSS_TIMER_BASE + 0x0014U)
#define TIMER_CPUC0                     (MSS_TIMER_BASE + 0x0018U)
#define TIMER_COMP0                     (MSS_TIMER_BASE + 0x0050U)
#define TIMER_UDCP0                     (MSS_TIMER_BASE + 0x0054U)

/* Counter 1 */
#define TIMER_FRC1                      (MSS_TIMER_BASE + 0x0030U)
#define TIMER_UC1                       (MSS_TIMER_BASE + 0x0034U)
#define TIMER_CPUC1                     (MSS_TIMER_BASE + 0x0038U)
#define TIMER_COMP1                     (MSS_TIMER_BASE + 0x0058U)
#define TIMER_UDCP1                     (MSS_TIMER_BASE + 0x005CU)

/* Timer interrupt */
#define TIMER_INTFLAG                   (MSS_TIMER_BASE + 0x0088U)
#define TIMER_INTEN_SET                 (MSS_TIMER_BASE + 0x008CU)
#define TIMER_INTEN_CLR                 (MSS_TIMER_BASE + 0x0090U)

#define TIMER_GCTRL_CNT0_EN            (0x00000001U)
#define TIMER_GCTRL_CNT1_EN            (0x00000002U)
#define TIMER_INT_COMP0                 (0x00000001U)
#define TIMER_INT_COMP1                 (0x00000002U)

/* ========================================================================== */
/*              OTP / eFuse (Trim Data)                                       */
/* ========================================================================== */

#define OTP_BASE                        (0x701C0000U)
#define OTP_TRIM_BASE                   (OTP_BASE + 0x0000U)
#define OTP_RF_TRIM_TX(ch)              (OTP_TRIM_BASE + 0x0100U + ((ch) * 0x10U))
#define OTP_RF_TRIM_RX(ch)              (OTP_TRIM_BASE + 0x0200U + ((ch) * 0x10U))
#define OTP_ADC_TRIM                    (OTP_TRIM_BASE + 0x0300U)
#define OTP_PLL_TRIM                    (OTP_TRIM_BASE + 0x0310U)
#define OTP_TEMP_SENSOR_TRIM            (OTP_TRIM_BASE + 0x0320U)
#define OTP_FW_VERSION                  (OTP_TRIM_BASE + 0x0400U)
#define OTP_CRC                         (OTP_TRIM_BASE + 0x04FCU)

/* ========================================================================== */
/*              ROM Patch Table                                               */
/* ========================================================================== */

#define ROM_PATCH_BASE                  (0x70100000U)
#define ROM_PATCH_CTRL                  (ROM_PATCH_BASE + 0x0000U)
#define ROM_PATCH_ADDR(n)               (ROM_PATCH_BASE + 0x0010U + ((n) * 0x08U))
#define ROM_PATCH_DATA(n)               (ROM_PATCH_BASE + 0x0014U + ((n) * 0x08U))
#define ROM_PATCH_ENABLE_MASK           (ROM_PATCH_BASE + 0x0200U)
#define ROM_PATCH_MAX_ENTRIES           32U

/* ========================================================================== */
/*              Interrupt Numbers                                             */
/* ========================================================================== */

#define INT_NUM_ESM_G1_HI               0U
#define INT_NUM_ESM_G2_HI               1U
#define INT_NUM_RTI_COMP0               2U
#define INT_NUM_RTI_COMP1               3U
#define INT_NUM_WDT_NMI                 4U
#define INT_NUM_DMA_CH0_DONE            16U
#define INT_NUM_DMA_CH1_DONE            17U
#define INT_NUM_SPI_INT                 32U
#define INT_NUM_GPIO_INT0               33U
#define INT_NUM_CHIRP_AVAIL             48U
#define INT_NUM_FRAME_START             49U
#define INT_NUM_FRAME_DONE              50U
#define INT_NUM_ADC_BUFFER_FULL         51U
#define INT_NUM_ECC_SEC                 64U
#define INT_NUM_ECC_DED                 65U

#endif /* AWR2944_REGS_H */
