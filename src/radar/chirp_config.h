/**
 * @file chirp_config.h
 * @brief Chirp Configuration Module Interface
 * @details Configures chirp profiles, frame parameters, and TX/RX channels
 *          for the AWR2944 FMCW radar chirp engine.
 * @copyright Copyright (c) 2024
 */
#ifndef CHIRP_CONFIG_H
#define CHIRP_CONFIG_H

#include "platform_types.h"

typedef struct {
    uint32   profileId;
    float32  startFreqGHz;
    float32  freqSlopeMHzUs;
    float32  idleTimeUs;
    float32  adcStartTimeUs;
    uint32   adcSamples;
    uint32   sampleRateMsps;
    uint32   txChannelMask;
    uint32   rxChannelMask;
} ChirpProfile_t;

typedef enum {
    FRAME_TRIGGER_SW   = 0U,
    FRAME_TRIGGER_HW   = 1U
} FrameTrigger_t;

typedef struct {
    uint32         numChirpsPerFrame;
    uint32         numFrames;
    float32        framePeriodMs;
    FrameTrigger_t triggerMode;
} FrameConfig_t;

Status_t ChirpConfig_SetProfile(const ChirpProfile_t *pProfile);
Status_t ChirpConfig_SetFrame(const FrameConfig_t *pFrame);
Status_t ChirpConfig_GetProfile(uint32 profileId, ChirpProfile_t *pProfile);

#endif /* CHIRP_CONFIG_H */
