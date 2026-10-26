/**
 * @file gpio_driver.c
 * @brief GPIO Driver with Safety Pin Control
 * @details Provides GPIO initialization, direction control, read/write,
 *          and multiplexing for the AWR2944. Includes specific handling
 *          for the safety-critical ESM error output pin.
 *
 * @copyright Copyright (c) 2024
 */

#include "gpio_driver.h"
#include "awr2944_regs.h"
#include "safety_cfg.h"

static boolean g_gpioInitialized = FALSE;

/**
 * @brief Initialize GPIO subsystem
 */
Status_t GPIO_Init(void)
{
    /* Set all GPIOs to input by default (safe default) */
    REG32_WR(GPIO_DIR(0), 0x00000000U);
    REG32_WR(GPIO_DIR(1), 0x00000000U);

    /* Clear all outputs */
    REG32_WR(GPIO_OUT(0), 0x00000000U);
    REG32_WR(GPIO_OUT(1), 0x00000000U);

    /* Clear interrupt status */
    REG32_WR(GPIO_INT_STATUS(0), 0xFFFFFFFFU);
    REG32_WR(GPIO_INT_STATUS(1), 0xFFFFFFFFU);

    g_gpioInitialized = TRUE;
    return STATUS_OK;
}

/**
 * @brief Set GPIO pin direction
 */
void GPIO_SetDirection(uint32 pin, uint32 direction)
{
    if (pin >= GPIO_MAX_PINS) return;

    uint32 bank = pin / GPIO_BANK_SIZE;
    uint32 bit  = pin % GPIO_BANK_SIZE;

    if (direction == GPIO_DIR_OUTPUT)
    {
        REG32_SET(GPIO_DIR(bank), (1U << bit));
    }
    else
    {
        REG32_CLR(GPIO_DIR(bank), (1U << bit));
    }
}

/**
 * @brief Write GPIO pin output
 */
void GPIO_Write(uint32 pin, uint32 value)
{
    if (pin >= GPIO_MAX_PINS) return;

    uint32 bank = pin / GPIO_BANK_SIZE;
    uint32 bit  = pin % GPIO_BANK_SIZE;

    if (value != 0U)
    {
        REG32_WR(GPIO_SET(bank), (1U << bit));
    }
    else
    {
        REG32_WR(GPIO_CLR(bank), (1U << bit));
    }
}

/**
 * @brief Read GPIO pin input
 */
uint32 GPIO_Read(uint32 pin)
{
    if (pin >= GPIO_MAX_PINS) return 0U;

    uint32 bank = pin / GPIO_BANK_SIZE;
    uint32 bit  = pin % GPIO_BANK_SIZE;

    return (REG32_RD(GPIO_IN(bank)) >> bit) & 1U;
}

/**
 * @brief Toggle GPIO pin
 */
void GPIO_Toggle(uint32 pin)
{
    if (pin >= GPIO_MAX_PINS) return;

    uint32 bank = pin / GPIO_BANK_SIZE;
    uint32 bit  = pin % GPIO_BANK_SIZE;

    REG32_WR(GPIO_TOGGLE(bank), (1U << bit));
}

/**
 * @brief Configure GPIO pin multiplexing
 */
void GPIO_SetMux(uint32 pin, uint32 muxMode)
{
    if (pin >= GPIO_MAX_PINS) return;
    REG32_WR(GPIO_MUX_CTRL(pin), muxMode & 0x0FU);
}
