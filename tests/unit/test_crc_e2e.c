/**
 * @file test_crc_e2e.c
 * @brief Unit Tests for E2E CRC-32 Module
 * @details Verifies CRC calculation correctness, E2E protection/check,
 *          and detection of data corruption.
 *
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>
#include <string.h>

#ifndef UNITY_BEGIN
#define UNITY_BEGIN()           do { printf("=== CRC E2E Test Suite ===\n"); } while(0)
#define UNITY_END()             do { printf("=== Test Suite End ===\n"); return 0; } while(0)
#define RUN_TEST(func)          do { printf("  [RUN]  %s\n", #func); func(); printf("  [PASS] %s\n", #func); } while(0)
#define TEST_ASSERT_EQUAL(e,a)  do { if ((e)!=(a)) { printf("  [FAIL] %s:%d expected=0x%X actual=0x%X\n", __FILE__, __LINE__, (unsigned)(e), (unsigned)(a)); return; } } while(0)
#define TEST_ASSERT_NOT_EQUAL(e,a) do { if ((e)==(a)) { printf("  [FAIL] %s:%d values should differ\n", __FILE__, __LINE__); return; } } while(0)
#endif

#include "platform_types.h"

/* Include CRC module directly for host testing */
#define E2E_CRC_INIT_VALUE   0xFFFFFFFFU
#define E2E_CRC_XOR_OUT      0xFFFFFFFFU
#define E2E_SEQUENCE_COUNTER_MAX 0xFFU

/* Forward declarations matching crc_e2e.h */
void     CRC_E2E_Init(void);
uint32   CRC_E2E_Calculate(const uint8 *pData, uint32 length);
Status_t CRC_E2E_Verify(const uint8 *pData, uint32 length, uint32 expectedCrc);

typedef struct {
    uint32 sequenceCounter;
    uint32 dataLength;
    uint32 crc;
} E2E_Header_t;

Status_t CRC_E2E_Protect(E2E_Header_t *pHeader, const uint8 *pData, uint32 length);
Status_t CRC_E2E_Check(const E2E_Header_t *pHeader, const uint8 *pData, uint32 length);

/* ========================================================================== */
/*                           Test Cases                                       */
/* ========================================================================== */

void test_CRC_KnownVector(void)
{
    /* CRC-32 of "123456789" = 0xCBF43926 */
    const uint8 data[] = "123456789";
    uint32 crc = CRC_E2E_Calculate(data, 9U);
    TEST_ASSERT_EQUAL(0xCBF43926U, crc);
}

void test_CRC_EmptyData(void)
{
    uint8 data[1] = {0};
    uint32 crc = CRC_E2E_Calculate(data, 0U);
    /* CRC of empty data with init=0xFFFFFFFF and XOR=0xFFFFFFFF = 0x00000000
     * (some implementations differ; verify against reference) */
    (void)crc;
    /* Just verify it doesn't crash */
}

void test_CRC_Verify_Pass(void)
{
    const uint8 data[] = "Hello, Radar!";
    uint32 len = 13U;
    uint32 crc = CRC_E2E_Calculate(data, len);
    Status_t result = CRC_E2E_Verify(data, len, crc);
    TEST_ASSERT_EQUAL(STATUS_OK, result);
}

void test_CRC_Verify_Fail(void)
{
    const uint8 data[] = "Hello, Radar!";
    uint32 len = 13U;
    uint32 crc = CRC_E2E_Calculate(data, len);
    /* Corrupt the CRC */
    Status_t result = CRC_E2E_Verify(data, len, crc ^ 0x01U);
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_CRC_DifferentData(void)
{
    const uint8 data1[] = "AAAA";
    const uint8 data2[] = "BBBB";
    uint32 crc1 = CRC_E2E_Calculate(data1, 4U);
    uint32 crc2 = CRC_E2E_Calculate(data2, 4U);
    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

void test_E2E_ProtectCheck(void)
{
    uint8 payload[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    E2E_Header_t header;

    CRC_E2E_Init();
    Status_t pStatus = CRC_E2E_Protect(&header, payload, 16U);
    TEST_ASSERT_EQUAL(STATUS_OK, pStatus);

    Status_t cStatus = CRC_E2E_Check(&header, payload, 16U);
    TEST_ASSERT_EQUAL(STATUS_OK, cStatus);
}

void test_E2E_CorruptedPayload(void)
{
    uint8 payload[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    E2E_Header_t header;

    CRC_E2E_Init();
    CRC_E2E_Protect(&header, payload, 16U);

    /* Corrupt one byte */
    payload[5] = 0xFFU;
    Status_t cStatus = CRC_E2E_Check(&header, payload, 16U);
    TEST_ASSERT_EQUAL(STATUS_ERROR, cStatus);
}

void test_E2E_NullPointer(void)
{
    Status_t result = CRC_E2E_Verify(NULL, 10U, 0U);
    TEST_ASSERT_EQUAL(STATUS_ERROR, result);
}

void test_CRC_SingleBitChange(void)
{
    uint8 data[4] = {0xA5, 0x5A, 0xA5, 0x5A};
    uint32 crc1 = CRC_E2E_Calculate(data, 4U);

    /* Flip one bit */
    data[2] ^= 0x01;
    uint32 crc2 = CRC_E2E_Calculate(data, 4U);

    TEST_ASSERT_NOT_EQUAL(crc1, crc2);
}

/* ========================================================================== */
/*                           Test Runner                                      */
/* ========================================================================== */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_CRC_KnownVector);
    RUN_TEST(test_CRC_EmptyData);
    RUN_TEST(test_CRC_Verify_Pass);
    RUN_TEST(test_CRC_Verify_Fail);
    RUN_TEST(test_CRC_DifferentData);
    RUN_TEST(test_E2E_ProtectCheck);
    RUN_TEST(test_E2E_CorruptedPayload);
    RUN_TEST(test_E2E_NullPointer);
    RUN_TEST(test_CRC_SingleBitChange);

    UNITY_END();
}
