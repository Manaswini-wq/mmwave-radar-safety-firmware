/**
 * @file control_flow_monitor.h
 * @brief Control Flow Monitoring Interface
 * @details Detects illegal program execution paths using signature
 *          accumulation at defined checkpoints.
 * @copyright Copyright (c) 2024
 */
#ifndef CONTROL_FLOW_MONITOR_H
#define CONTROL_FLOW_MONITOR_H

#include "platform_types.h"

/* Checkpoint IDs */
#define CFM_CP_SAFETY_INIT_DONE   0U
#define CFM_CP_ROM_PATCH_DONE     1U
#define CFM_CP_HW_INIT_DONE       2U
#define CFM_CP_SELFTEST_DONE      3U
#define CFM_CP_CAL_DONE           4U
#define CFM_CP_LOOP_START         5U
#define CFM_CP_LOOP_END           6U

void     CFM_Init(uint32 seed);
void     CFM_Checkpoint(uint32 checkpointId);
Status_t CFM_Verify(void);
void     CFM_Reset(void);

#endif /* CONTROL_FLOW_MONITOR_H */
