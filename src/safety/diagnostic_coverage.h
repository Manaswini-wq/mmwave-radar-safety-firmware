/**
 * @file diagnostic_coverage.h
 * @brief Diagnostic Test Runner Interface
 * @details Provides diagnostic test functions for comprehensive coverage
 *          per ISO 26262 Table 11 and IEC 61508 Table A.1-A.17.
 *
 * @copyright Copyright (c) 2024
 */
#ifndef DIAGNOSTIC_COVERAGE_H
#define DIAGNOSTIC_COVERAGE_H

#include "platform_types.h"

DiagResult_t Diagnostic_CPURegisterTest(void);
DiagResult_t Diagnostic_PCIntegrityTest(void);
DiagResult_t Diagnostic_StackOverflowCheck(void);
DiagResult_t Diagnostic_RAMMarchTest(void);
DiagResult_t Diagnostic_VoltageMonitor(void);
DiagResult_t Diagnostic_TemperatureMonitor(void);
DiagResult_t Diagnostic_PLLLockCheck(void);
DiagResult_t Diagnostic_ADCLoopbackTest(void);
DiagResult_t Diagnostic_DMAIntegrityTest(void);
DiagResult_t Diagnostic_ControlFlowCheck(void);

#endif /* DIAGNOSTIC_COVERAGE_H */
