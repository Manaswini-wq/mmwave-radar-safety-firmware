/**
 * @file gpio_driver.h
 * @brief GPIO Driver Interface with Safety Pin Control
 * @copyright Copyright (c) 2024
 */
#ifndef GPIO_DRIVER_H
#define GPIO_DRIVER_H

#include "platform_types.h"

#define GPIO_DIR_INPUT   0U
#define GPIO_DIR_OUTPUT  1U

Status_t GPIO_Init(void);
void GPIO_SetDirection(uint32 pin, uint32 direction);
void GPIO_Write(uint32 pin, uint32 value);
uint32 GPIO_Read(uint32 pin);
void GPIO_Toggle(uint32 pin);
void GPIO_SetMux(uint32 pin, uint32 muxMode);

#endif /* GPIO_DRIVER_H */
