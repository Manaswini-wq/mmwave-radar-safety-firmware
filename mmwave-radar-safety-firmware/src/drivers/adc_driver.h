/**
 * @file adc_driver.h
 * @brief ADC Driver Interface for AWR2944 Mixed-Signal SOC
 * @copyright Copyright (c) 2024
 */
#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "platform_types.h"

typedef enum {
    ADC_RES_12BIT = 0U,
    ADC_RES_14BIT = 1U,
    ADC_RES_16BIT = 2U
} ADC_Resolution_t;

typedef enum {
    ADC_FMT_REAL    = 0U,
    ADC_FMT_COMPLEX = 1U
} ADC_Format_t;

Status_t ADC_Init(void);
Status_t ADC_Configure(ADC_Resolution_t resolution, ADC_Format_t format, uint32 sampleRateMsps);
Status_t ADC_Start(void);
Status_t ADC_Stop(void);
Status_t ADC_EnableLoopback(void);
Status_t ADC_DisableLoopback(void);
uint32   ADC_ReadSample(uint32 channel);

#endif /* ADC_DRIVER_H */
