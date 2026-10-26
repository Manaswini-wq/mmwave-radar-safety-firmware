/**
 * @file hal_interrupt.c
 * @brief HAL Interrupt Controller (VIM) Implementation
 * @details Manages the Vectored Interrupt Manager (VIM) on the AWR2944.
 *          Provides ISR registration, enable/disable, and dispatch.
 *
 * @copyright Copyright (c) 2024
 */

#include "hal_interrupt.h"
#include "awr2944_regs.h"

static ISR_Handler_t g_isrTable[VIM_MAX_INTERRUPTS];

/**
 * @brief Initialize the VIM interrupt controller
 */
void HAL_Interrupt_Init(void)
{
    uint32 i;

    /* Clear all ISR handlers */
    for (i = 0U; i < VIM_MAX_INTERRUPTS; i++)
    {
        g_isrTable[i] = NULL_PTR;
    }

    /* Disable all interrupts */
    for (i = 0U; i < 8U; i++)
    {
        REG32_WR(VIM_INTR_EN_CLR(i), 0xFFFFFFFFU);
        REG32_WR(VIM_STS(i), 0xFFFFFFFFU);  /* Clear pending */
    }

    /* Set default priority (0 = highest) for all interrupts */
    for (i = 0U; i < VIM_MAX_INTERRUPTS; i++)
    {
        REG32_WR(VIM_PRI(i), 15U);  /* Lowest priority default */
    }
}

/**
 * @brief Register an ISR handler for an interrupt number
 */
void HAL_Interrupt_Register(uint32 intNum, ISR_Handler_t handler)
{
    if (intNum >= VIM_MAX_INTERRUPTS) return;

    g_isrTable[intNum] = handler;

    /* Set vector address (not used in our dispatch model, but program it) */
    REG32_WR(VIM_VEC(intNum), (uint32)handler);

    /* Set high priority for registered interrupts */
    REG32_WR(VIM_PRI(intNum), 0U);

    /* Enable the interrupt */
    HAL_Interrupt_Enable(intNum);
}

/**
 * @brief Enable a specific interrupt
 */
void HAL_Interrupt_Enable(uint32 intNum)
{
    if (intNum >= VIM_MAX_INTERRUPTS) return;
    uint32 group = intNum / 32U;
    uint32 bit   = intNum % 32U;
    REG32_WR(VIM_INTR_EN_SET(group), (1U << bit));
}

/**
 * @brief Disable a specific interrupt
 */
void HAL_Interrupt_Disable(uint32 intNum)
{
    if (intNum >= VIM_MAX_INTERRUPTS) return;
    uint32 group = intNum / 32U;
    uint32 bit   = intNum % 32U;
    REG32_WR(VIM_INTR_EN_CLR(group), (1U << bit));
}

/**
 * @brief Enable global IRQ
 */
void HAL_Interrupt_GlobalEnable(void)
{
    __asm volatile("CPSIE I");
}

/**
 * @brief Disable global IRQ
 */
void HAL_Interrupt_GlobalDisable(void)
{
    __asm volatile("CPSID I");
}

/**
 * @brief Dispatch an interrupt to the registered handler
 * @details Called from the IRQ vector in vector_table.S
 */
void HAL_Interrupt_Dispatch(uint32 intNum)
{
    if ((intNum < VIM_MAX_INTERRUPTS) && (g_isrTable[intNum] != NULL_PTR))
    {
        g_isrTable[intNum]();
    }
}
