/**
 * @file safety_cfg.h
 * @brief Safety configuration for ISO 26262 ASIL-D and IEC 61508 SIL-3
 * @details Compile-time configuration for fault-tolerant, fail-safe architecture
 *          with comprehensive diagnostic coverage and safety mechanisms.
 *
 *          All thresholds and intervals are derived from the FMEA/FTA analysis
 *          and verified against the Safety Manual requirements.
 *
 * @copyright Copyright (c) 2024
 * @license MIT
 */
#ifndef SAFETY_CFG_H
#define SAFETY_CFG_H

#include "platform_types.h"

/* ========================================================================== */
/*                  ISO 26262 ASIL-D Configuration                            */
/* ========================================================================== */

#define SAFETY_ASIL_LEVEL               SAFETY_ASIL_D
#define SAFETY_SPFM_TARGET_PERCENT      99U    /**< Single-Point Fault Metric >99%  */
#define SAFETY_LFM_TARGET_PERCENT       97U    /**< Latent Fault Metric >97%        */

/* ========================================================================== */
/*                  IEC 61508 SIL-3 Configuration                             */
/* ========================================================================== */

#define SAFETY_SIL_LEVEL                3U
#define SAFETY_SFF_TARGET_PERCENT       99U    /**< Safe Failure Fraction            */

/* ========================================================================== */
/*                  Watchdog Configuration                                    */
/* ========================================================================== */

#define WDT_TIMEOUT_MS                  10U    /**< Watchdog timeout period          */
#define WDT_WINDOW_MIN_MS               2U     /**< Minimum window for windowed WDT */
#define WDT_WINDOW_MAX_MS               8U     /**< Maximum window for windowed WDT */
#define WDT_SERVICE_PATTERN             0xA35CU /**< Watchdog service key            */
#define WDT_SERVICE_PATTERN_INV         0x5CA3U /**< Inverted service key            */

/* ========================================================================== */
/*                  ECC Configuration                                         */
/* ========================================================================== */

#define ECC_TCMA_ENABLE                 TRUE
#define ECC_TCMB_ENABLE                 TRUE
#define ECC_L2_ENABLE                   TRUE
#define ECC_VIM_RAM_ENABLE              TRUE
#define ECC_SEC_THRESHOLD               3U     /**< SEC error threshold before alert */
#define ECC_DED_ACTION_RESET            TRUE   /**< DED triggers system reset        */

/* ========================================================================== */
/*                  Lockstep CPU Configuration                                */
/* ========================================================================== */

#define LOCKSTEP_ENABLE                 TRUE
#define LOCKSTEP_COMPARE_TIMEOUT_US     1U     /**< Max delay for compare result     */
#define LOCKSTEP_SELFTEST_INTERVAL_MS   100U   /**< Self-test injection interval     */

/* ========================================================================== */
/*                  Control Flow Monitoring                                   */
/* ========================================================================== */

#define CFM_ENABLE                      TRUE
#define CFM_CHECKPOINT_COUNT            32U    /**< Max number of checkpoints        */
#define CFM_SIGNATURE_SEED              0xDEADBEEFU /**< Initial CFM signature       */
#define CFM_EXPECTED_PATH_LENGTH        16U    /**< Expected main-loop path length   */

/* ========================================================================== */
/*                  BIST Configuration                                        */
/* ========================================================================== */

#define PBIST_ENABLE                    TRUE   /**< Programmable BIST for memories   */
#define LBIST_ENABLE                    TRUE   /**< Logic BIST for CPU core          */
#define BIST_INTERVAL_MS                100U   /**< Periodic BIST interval           */
#define PBIST_ALGO_MARCH13N             TRUE   /**< Use March-13N algorithm          */

/* ========================================================================== */
/*                  E2E CRC Protection                                        */
/* ========================================================================== */

#define E2E_CRC_ENABLE                  TRUE
#define E2E_CRC_POLYNOMIAL              0x04C11DB7U  /**< CRC-32 (IEEE 802.3)       */
#define E2E_CRC_INIT_VALUE              0xFFFFFFFFU  /**< Initial CRC register value */
#define E2E_CRC_XOR_OUT                 0xFFFFFFFFU  /**< Final XOR value            */
#define E2E_SEQUENCE_COUNTER_MAX        0xFFU        /**< 8-bit alive counter        */

/* ========================================================================== */
/*                  ESM (Error Signaling Module) Configuration                */
/* ========================================================================== */

#define ESM_GROUP1_ENABLE               TRUE   /**< Low-priority error group         */
#define ESM_GROUP2_ENABLE               TRUE   /**< High-priority error group        */
#define ESM_GROUP3_ENABLE               TRUE   /**< Direct-to-pin error group        */
#define ESM_SAFE_STATE_PIN              42U    /**< GPIO pin for external safe-state */
#define ESM_ERROR_PIN_ACTIVE_LOW        TRUE   /**< nERROR pin polarity              */

/* ========================================================================== */
/*                  Diagnostic Coverage Intervals                             */
/* ========================================================================== */

/** @defgroup DIAG_INTERVALS Diagnostic Test Intervals
 *  Intervals derived from FMEA fault exposure time analysis.
 *  All values in milliseconds.
 *  @{
 */
#define DIAG_CPU_TEST_INTERVAL_MS       10U    /**< CPU register / ALU test          */
#define DIAG_RAM_TEST_INTERVAL_MS       50U    /**< RAM march test (background)      */
#define DIAG_ADC_TEST_INTERVAL_MS       100U   /**< ADC loopback self-test           */
#define DIAG_VOLTAGE_MON_INTERVAL_MS    20U    /**< Supply voltage monitor           */
#define DIAG_TEMP_MON_INTERVAL_MS       200U   /**< Die temperature monitor          */
#define DIAG_PLL_LOCK_INTERVAL_MS       10U    /**< PLL lock status check            */
#define DIAG_DMA_TEST_INTERVAL_MS       100U   /**< DMA channel integrity test       */
#define DIAG_CFM_CHECK_INTERVAL_MS      10U    /**< Control flow monitor check       */
/** @} */

/* ========================================================================== */
/*                  Safe State Configuration                                  */
/* ========================================================================== */

#define SAFE_STATE_TX_DISABLE           TRUE   /**< Disable all TX channels          */
#define SAFE_STATE_CHIRP_STOP           TRUE   /**< Stop chirp sequencer             */
#define SAFE_STATE_NOTIFY_HOST          TRUE   /**< Send fault notification to host  */
#define SAFE_STATE_MAX_RECOVERY_MS      50U    /**< Max time for recovery attempt    */
#define SAFE_STATE_MAX_RETRIES          3U     /**< Max recovery attempts            */
#define SAFE_STATE_ESCALATION_RESET     TRUE   /**< Reset on recovery failure        */

/* ========================================================================== */
/*                  Radar Operating Parameters                                */
/* ========================================================================== */

#define RADAR_MAX_TX_CHANNELS           4U     /**< AWR2944 has 4 TX antennas        */
#define RADAR_MAX_RX_CHANNELS           4U     /**< AWR2944 has 4 RX antennas        */
#define RADAR_MAX_CHIRP_PROFILES        16U    /**< Max chirp profile count           */
#define RADAR_MAX_ADC_SAMPLES           1024U  /**< Max ADC samples per chirp         */
#define RADAR_ADC_SAMPLE_RATE_MSPS      37U    /**< Max ADC sample rate (MSPS)        */

#endif /* SAFETY_CFG_H */
