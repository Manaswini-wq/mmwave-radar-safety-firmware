/**
 * @file radar_control.h
 * @brief Real-Time Radar Control Interface
 * @details Controls radar power-on sequence, frame/chirp configuration,
 *          start/stop, and status monitoring for the AWR2944 chirp engine.
 * @copyright Copyright (c) 2024
 */
#ifndef RADAR_CONTROL_H
#define RADAR_CONTROL_H

#include "platform_types.h"

typedef enum {
    RADAR_STATE_OFF       = 0U,
    RADAR_STATE_INIT      = 1U,
    RADAR_STATE_IDLE      = 2U,
    RADAR_STATE_ACTIVE    = 3U,
    RADAR_STATE_ERROR     = 4U
} RadarState_t;

Status_t    RadarControl_Init(void);
Status_t    RadarControl_PowerOn(void);
Status_t    RadarControl_PowerOff(void);
Status_t    RadarControl_StartFrames(void);
Status_t    RadarControl_StopFrames(void);
RadarState_t RadarControl_GetState(void);
void        RadarControl_ChirpDoneISR(void);
void        RadarControl_FrameDoneISR(void);

#endif /* RADAR_CONTROL_H */
