/**
 * @file crc_e2e.h
 * @brief End-to-End CRC-32 Protection Interface
 * @details Provides CRC-32 computation and verification for data integrity
 *          protection across the radar data path.
 * @copyright Copyright (c) 2024
 */
#ifndef CRC_E2E_H
#define CRC_E2E_H

#include "platform_types.h"

void     CRC_E2E_Init(void);
uint32   CRC_E2E_Calculate(const uint8 *pData, uint32 length);
Status_t CRC_E2E_Verify(const uint8 *pData, uint32 length, uint32 expectedCrc);

/** @brief Data packet with E2E CRC protection */
typedef struct {
    uint32  sequenceCounter;   /**< Alive counter for freshness */
    uint32  dataLength;        /**< Payload length in bytes     */
    uint32  crc;               /**< CRC-32 of header + payload  */
} E2E_Header_t;

Status_t CRC_E2E_Protect(E2E_Header_t *pHeader, const uint8 *pData, uint32 length);
Status_t CRC_E2E_Check(const E2E_Header_t *pHeader, const uint8 *pData, uint32 length);

#endif /* CRC_E2E_H */
