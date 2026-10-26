/**
 * @file watchdog.h
 * @brief Windowed Watchdog Driver Interface
 * @details Provides windowed watchdog timer control for ISO 26262 ASIL-D.
 *          Supports window violation detection and fail-safe reset trigger.
 * @copyright Copyright (c) 2024
 */
#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "platform_types.h"

Status_t Watchdog_Init(uint32 timeoutMs, uint32 windowMinMs);
Status_t Watchdog_Enable(void);
Status_t Watchdog_Disable(void);
void     Watchdog_Service(void);
boolean  Watchdog_IsRunning(void);
boolean  Watchdog_CheckViolation(void);

#endif /* WATCHDOG_H */
