/**
 * @file dac_driver.c
 * @brief DAC Driver for AWR2944 Mixed-Signal SOC
 * @details Controls the on-chip DAC used for analog test signal
 *          generation, DC offset compensation, and calibration
 *          reference in the radar front-end.
 *
 * @copyright Copyright (c) 2024
 */

#include "dac_driver.h"
#include "awr2944_regs.h"

static boolean g_dacInitialized = FALSE;

/**
 * @brief Initialize the DAC subsystem
 */
Status_t DAC_Init(void)
{
    /* Reset DAC */
    REG32_WR(DAC_CTRL, DAC_CTRL_RESET);
    volatile uint32 d;
    for (d = 0U; d < 500U; d++) { /* spin */ }
    REG32_CLR(DAC_CTRL, DAC_CTRL_RESET);

    /* Set default gain and offset */
    REG32_WR(DAC_GAIN, 0x80U);      /* Mid-scale gain */
    REG32_WR(DAC_OFFSET, 0x00U);    /* Zero offset */
    REG32_WR(DAC_CLK_DIV, 0x04U);   /* Clock divider */
    REG32_WR(DAC_DATA, 0x800U);     /* Mid-code output */

    g_dacInitialized = TRUE;
    return STATUS_OK;
}

Status_t DAC_SetOutput(uint32 value)
{
    if (g_dacInitialized != TRUE) return STATUS_NOT_READY;
    REG32_WR(DAC_DATA, value & 0x0FFFU);
    return STATUS_OK;
}

Status_t DAC_SetGain(uint32 gainCode)
{
    if (g_dacInitialized != TRUE) return STATUS_NOT_READY;
    REG32_WR(DAC_GAIN, gainCode & 0xFFU);
    return STATUS_OK;
}

Status_t DAC_SetOffset(int32 offsetCode)
{
    if (g_dacInitialized != TRUE) return STATUS_NOT_READY;
    REG32_WR(DAC_OFFSET, (uint32)(offsetCode & 0xFFU));
    return STATUS_OK;
}

Status_t DAC_Enable(void)
{
    if (g_dacInitialized != TRUE) return STATUS_NOT_READY;
    REG32_SET(DAC_CTRL, DAC_CTRL_ENABLE);
    return STATUS_OK;
}

Status_t DAC_Disable(void)
{
    REG32_CLR(DAC_CTRL, DAC_CTRL_ENABLE);
    return STATUS_OK;
}
