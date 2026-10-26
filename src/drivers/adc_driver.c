/**
 * @file adc_driver.c
 * @brief ADC Driver for AWR2944 Mixed-Signal Analog and Digital SOC
 * @details Provides initialization, configuration, and control of the
 *          on-chip ADC used for IF (Intermediate Frequency) signal
 *          digitization in the radar receive path. Supports 12/14/16-bit
 *          resolution, real and complex (I/Q) output formats, and
 *          internal loopback test mode for diagnostic self-tests.
 *
 *          The ADC operates at up to 37.5 MSPS and feeds the DMA engine
 *          for zero-copy data transfer to processing buffers.
 *
 * @copyright Copyright (c) 2024
 */

#include "adc_driver.h"
#include "awr2944_regs.h"
#include "safety_cfg.h"

/* ========================================================================== */
/*                           Module Variables                                 */
/* ========================================================================== */

static boolean g_adcInitialized = FALSE;
static ADC_Resolution_t g_currentRes = ADC_RES_12BIT;
static ADC_Format_t g_currentFmt = ADC_FMT_REAL;
static uint32 g_sampleRate = 25U;

/* ========================================================================== */
/*                       Public Functions                                     */
/* ========================================================================== */

/**
 * @brief Initialize the ADC subsystem
 * @details Resets the ADC block, configures default parameters,
 *          and enables the analog bias circuits.
 * @return STATUS_OK on success
 */
Status_t ADC_Init(void)
{
    /* Reset ADC */
    REG32_WR(ADC_CTRL, ADC_CTRL_RESET);

    /* Wait for reset completion */
    volatile uint32 delay;
    for (delay = 0U; delay < 1000U; delay++) { /* spin */ }

    /* Clear reset */
    REG32_CLR(ADC_CTRL, ADC_CTRL_RESET);

    /* Configure default: 12-bit, real format, 25 MSPS */
    REG32_WR(ADC_RESOLUTION, ADC_RES_12BIT);
    REG32_WR(ADC_DATA_FORMAT, ADC_FORMAT_REAL);
    REG32_WR(ADC_SAMPLE_RATE, 25U);

    /* Set default number of samples */
    REG32_WR(ADC_NUM_SAMPLES, 256U);

    /* Configure clock divider for sample rate */
    /* ADC_CLK = RFSS_CLK / (divider + 1), target = sample_rate */
    uint32 rfssClkMHz = 200U;
    uint32 divider = (rfssClkMHz / 25U) - 1U;
    REG32_WR(ADC_CLK_DIV, divider);

    /* Clear all interrupt status */
    REG32_WR(ADC_INT_CLEAR, 0xFFFFFFFFU);

    /* Disable loopback by default */
    REG32_CLR(ADC_LOOPBACK_CTRL, ADC_CTRL_LOOPBACK);

    g_currentRes = ADC_RES_12BIT;
    g_currentFmt = ADC_FMT_REAL;
    g_sampleRate = 25U;
    g_adcInitialized = TRUE;

    return STATUS_OK;
}

/**
 * @brief Configure ADC parameters
 * @param resolution ADC resolution (12/14/16 bit)
 * @param format Output format (real or complex I/Q)
 * @param sampleRateMsps Sample rate in MSPS
 * @return STATUS_OK on success
 */
Status_t ADC_Configure(ADC_Resolution_t resolution, ADC_Format_t format,
                        uint32 sampleRateMsps)
{
    if (g_adcInitialized != TRUE)
    {
        return STATUS_NOT_READY;
    }

    if (sampleRateMsps > RADAR_ADC_SAMPLE_RATE_MSPS)
    {
        return STATUS_ERROR;
    }

    /* Stop ADC before reconfiguration */
    REG32_CLR(ADC_CTRL, ADC_CTRL_ENABLE);

    /* Set resolution */
    REG32_WR(ADC_RESOLUTION, (uint32)resolution);

    /* Set data format */
    REG32_WR(ADC_DATA_FORMAT, (uint32)format);

    /* Set sample rate */
    REG32_WR(ADC_SAMPLE_RATE, sampleRateMsps);

    /* Recalculate clock divider */
    uint32 rfssClkMHz = 200U;
    uint32 divider = (rfssClkMHz / sampleRateMsps) - 1U;
    REG32_WR(ADC_CLK_DIV, divider);

    g_currentRes = resolution;
    g_currentFmt = format;
    g_sampleRate = sampleRateMsps;

    return STATUS_OK;
}

/**
 * @brief Start ADC conversion
 */
Status_t ADC_Start(void)
{
    if (g_adcInitialized != TRUE)
    {
        return STATUS_NOT_READY;
    }

    REG32_SET(ADC_CTRL, ADC_CTRL_ENABLE);
    return STATUS_OK;
}

/**
 * @brief Stop ADC conversion
 */
Status_t ADC_Stop(void)
{
    REG32_CLR(ADC_CTRL, ADC_CTRL_ENABLE);
    return STATUS_OK;
}

/**
 * @brief Enable ADC internal loopback for diagnostic testing
 */
Status_t ADC_EnableLoopback(void)
{
    REG32_SET(ADC_LOOPBACK_CTRL, ADC_CTRL_LOOPBACK);
    return STATUS_OK;
}

/**
 * @brief Disable ADC loopback
 */
Status_t ADC_DisableLoopback(void)
{
    REG32_CLR(ADC_LOOPBACK_CTRL, ADC_CTRL_LOOPBACK);
    return STATUS_OK;
}

/**
 * @brief Read a single ADC sample from the buffer
 * @param channel RX channel index (0-3)
 * @return Raw ADC sample value
 */
uint32 ADC_ReadSample(uint32 channel)
{
    if (channel >= RADAR_MAX_RX_CHANNELS)
    {
        return 0U;
    }

    return REG32_RD(ADC_BUFFER_BASE + (channel * 4U));
}
