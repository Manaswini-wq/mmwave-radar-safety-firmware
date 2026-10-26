/**
 * @file lockstep_monitor.h
 * @brief Lockstep CPU Monitor Interface
 * @details Monitors the R5F lockstep CPU compare logic for ISO 26262 ASIL-D.
 * @copyright Copyright (c) 2024
 */
#ifndef LOCKSTEP_MONITOR_H
#define LOCKSTEP_MONITOR_H

#include "platform_types.h"

Status_t     Lockstep_Init(void);
DiagResult_t Lockstep_SelfTest(void);
DiagResult_t Lockstep_GetStatus(void);
boolean      Lockstep_IsEnabled(void);

#endif /* LOCKSTEP_MONITOR_H */
