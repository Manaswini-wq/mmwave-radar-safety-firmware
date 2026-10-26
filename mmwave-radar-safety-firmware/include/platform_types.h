/**
 * @file platform_types.h
 * @brief Platform type definitions for ISO 26262 ASIL-D compliant firmware
 * @details Defines standard types per AUTOSAR platform types specification
 *          for TI AWR2944 mmWave radar sensor bare-metal firmware.
 *
 * @copyright Copyright (c) 2024
 * @license MIT
 *
 * MISRA C:2012 Compliant -- Advisory Rule 4.6: typedefs used for fixed-width types
 */
#ifndef PLATFORM_TYPES_H
#define PLATFORM_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* ========================================================================== */
/*                          Fixed-Width Integer Types                          */
/* ========================================================================== */

typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int8_t    int8;
typedef int16_t   int16;
typedef int32_t   int32;
typedef int64_t   int64;
typedef float     float32;
typedef double    float64;

/* ========================================================================== */
/*                        Register Access Types                               */
/* ========================================================================== */

typedef volatile uint32  reg32_t;
typedef volatile uint16  reg16_t;

/* ========================================================================== */
/*                        Boolean Type (Safety-Critical)                      */
/* ========================================================================== */

/** @brief Boolean type for safety-critical code (explicit width) */
typedef uint8 boolean;
#define TRUE  ((boolean)1U)
#define FALSE ((boolean)0U)

/* ========================================================================== */
/*                        Return Status Codes                                 */
/* ========================================================================== */

/**
 * @brief Return status for safety functions
 * @details Values chosen with complementary bit patterns for fault detection.
 *          Any bit-flip in status storage is detectable by pattern check.
 */
typedef enum {
    STATUS_OK             = 0x5AU,   /**< Operation completed successfully           */
    STATUS_ERROR          = 0xA5U,   /**< General error                              */
    STATUS_SAFETY_FAULT   = 0x3CU,   /**< Safety mechanism detected a fault          */
    STATUS_TIMEOUT        = 0xC3U,   /**< Operation timed out                        */
    STATUS_NOT_READY      = 0x69U    /**< Module not initialized / not ready         */
} Status_t;

/* ========================================================================== */
/*                        Safety Integrity Levels                             */
/* ========================================================================== */

/**
 * @brief ISO 26262 ASIL levels
 */
typedef enum {
    SAFETY_QM     = 0U,
    SAFETY_ASIL_A = 1U,
    SAFETY_ASIL_B = 2U,
    SAFETY_ASIL_C = 3U,
    SAFETY_ASIL_D = 4U
} SafetyLevel_t;

/* ========================================================================== */
/*                        Diagnostic Result Type                              */
/* ========================================================================== */

/**
 * @brief Diagnostic test result codes
 * @details Complementary bit patterns enable stuck-at fault detection on
 *          the result storage variable.
 */
typedef enum {
    DIAG_PASS    = 0xAAU,
    DIAG_FAIL    = 0x55U,
    DIAG_PENDING = 0x00U
} DiagResult_t;

/* ========================================================================== */
/*                        NULL Pointer Guard                                  */
/* ========================================================================== */

#ifndef NULL_PTR
#define NULL_PTR  ((void *)0)
#endif

/* ========================================================================== */
/*                        Compiler Attribute Macros                           */
/* ========================================================================== */

#define SAFETY_CRITICAL   __attribute__((section(".safety_critical")))
#define PACKED            __attribute__((packed))
#define ALIGNED(x)        __attribute__((aligned(x)))
#define NORETURN          __attribute__((noreturn))
#define WEAK              __attribute__((weak))
#define INLINE            static inline __attribute__((always_inline))
#define NOINLINE          __attribute__((noinline))

#endif /* PLATFORM_TYPES_H */
