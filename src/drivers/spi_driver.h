/**
 * @file spi_driver.h
 * @brief SPI Driver Interface for Host Communication
 * @copyright Copyright (c) 2024
 */
#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include "platform_types.h"

Status_t SPI_Init(void);
Status_t SPI_TransmitByte(uint8 data);
Status_t SPI_ReceiveByte(uint8 *pData);
Status_t SPI_Transfer(const uint8 *pTxBuf, uint8 *pRxBuf, uint32 length);
boolean  SPI_IsBusy(void);

#endif /* SPI_DRIVER_H */
