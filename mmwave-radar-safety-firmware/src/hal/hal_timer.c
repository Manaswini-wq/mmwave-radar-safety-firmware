/**
 * @file hal_timer.c
 * @brief HAL Timer (RTI) Implementation
 * @details Manages the RTI (Real-Time Interrupt) timer module for
 *          system tick generation and periodic diagnostic scheduling.
 *
 * @copyright Copyright (c) 2024
 */

#include "hal_timer.h"
#include "awr2944_regs.h"

#define RTI_CLK_MHZ  200U

/**
 * @brief Initialize a timer with given period
 * @param timerId Timer index (0 or 1)
 * @param periodUs Timer period in microseconds
 */
void HAL_Timer_Init(uint32 timerId, uint32 periodUs)
{
    /* Stop timer before configuration */
    HAL_Timer_Stop(timerId);

    /* Calculate compare value */
    uint32 compareVal = periodUs * RTI_CLK_MHZ;

    if (timerId == 0U)
    {
        /* Configure counter 0 prescaler (divide-by-1) */
        REG32_WR(TIMER_CPUC0, 0U);

        /* Set compare value */
        REG32_WR(TIMER_COMP0, compareVal);

        /* Set update compare value for periodic mode */
        REG32_WR(TIMER_UDCP0, compareVal);

        /* Enable compare 0 interrupt */
        REG32_WR(TIMER_INTEN_SET, TIMER_INT_COMP0);
    }
    else if (timerId == 1U)
    {
        REG32_WR(TIMER_CPUC1, 0U);
        REG32_WR(TIMER_COMP1, compareVal);
        REG32_WR(TIMER_UDCP1, compareVal);
        REG32_WR(TIMER_INTEN_SET, TIMER_INT_COMP1);
    }
}

/**
 * @brief Start a timer
 */
void HAL_Timer_Start(uint32 timerId)
{
    if (timerId == 0U)
    {
        REG32_SET(TIMER_GCTRL, TIMER_GCTRL_CNT0_EN);
    }
    else if (timerId == 1U)
    {
        REG32_SET(TIMER_GCTRL, TIMER_GCTRL_CNT1_EN);
    }
}

/**
 * @brief Stop a timer
 */
void HAL_Timer_Stop(uint32 timerId)
{
    if (timerId == 0U)
    {
        REG32_CLR(TIMER_GCTRL, TIMER_GCTRL_CNT0_EN);
    }
    else if (timerId == 1U)
    {
        REG32_CLR(TIMER_GCTRL, TIMER_GCTRL_CNT1_EN);
    }
}

/**
 * @brief Get current counter value
 */
uint32 HAL_Timer_GetCount(uint32 timerId)
{
    if (timerId == 0U)
    {
        return REG32_RD(TIMER_FRC0);
    }
    else if (timerId == 1U)
    {
        return REG32_RD(TIMER_FRC1);
    }
    return 0U;
}

/**
 * @brief Clear timer compare interrupt flag
 */
void HAL_Timer_ClearInterrupt(uint32 timerId)
{
    if (timerId == 0U)
    {
        REG32_WR(TIMER_INTFLAG, TIMER_INT_COMP0);
    }
    else if (timerId == 1U)
    {
        REG32_WR(TIMER_INTFLAG, TIMER_INT_COMP1);
    }
}
