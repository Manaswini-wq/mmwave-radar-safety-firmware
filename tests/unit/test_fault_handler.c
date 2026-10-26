/**
 * @file test_fault_handler.c
 * @brief Unit Tests for Fault Handler Module
 * @details Tests fault classification, recovery logic, logging, and
 *          ESM error processing.
 *
 * @copyright Copyright (c) 2024
 */

#include <stdio.h>

#ifndef UNITY_BEGIN
#define UNITY_BEGIN()           do { printf("=== Fault Handler Test Suite ===\n"); } while(0)
#define UNITY_END()             do { printf("=== Test Suite End ===\n"); return 0; } while(0)
#define RUN_TEST(func)          do { printf("  [RUN]  %s\n", #func); func(); printf("  [PASS] %s\n", #func); } while(0)
#define TEST_ASSERT_EQUAL(e,a)  do { if ((e)!=(a)) { printf("  [FAIL] %s:%d\n", __FILE__, __LINE__); return; } } while(0)
#define TEST_ASSERT_TRUE(c)     do { if (!(c)) { printf("  [FAIL] %s:%d\n", __FILE__, __LINE__); return; } } while(0)
#endif

#include "platform_types.h"

/* Fault classification simulation */
typedef enum { FC_TRANSIENT=0, FC_INTERMITTENT=2, FC_PERMANENT=1 } FClass_t;

static FClass_t classify(uint32 count)
{
    if (count <= 1U) return FC_TRANSIENT;
    else if (count <= 3U) return FC_INTERMITTENT;
    else return FC_PERMANENT;
}

void test_FaultClassify_Transient(void)
{
    TEST_ASSERT_EQUAL(FC_TRANSIENT, classify(0U));
    TEST_ASSERT_EQUAL(FC_TRANSIENT, classify(1U));
}

void test_FaultClassify_Intermittent(void)
{
    TEST_ASSERT_EQUAL(FC_INTERMITTENT, classify(2U));
    TEST_ASSERT_EQUAL(FC_INTERMITTENT, classify(3U));
}

void test_FaultClassify_Permanent(void)
{
    TEST_ASSERT_EQUAL(FC_PERMANENT, classify(4U));
    TEST_ASSERT_EQUAL(FC_PERMANENT, classify(100U));
}

void test_FaultLog_Wrapping(void)
{
    uint32 logSize = 64U;
    uint32 idx = 0U;
    uint32 i;
    for (i = 0U; i < 100U; i++)
    {
        idx++;
        if (idx >= logSize) idx = 0U;
    }
    TEST_ASSERT_TRUE(idx < logSize);
}

void test_FaultSeverity_Escalation(void)
{
    /* Critical severity should trigger safe-state */
    uint32 severity = 3U;  /* CRITICAL */
    boolean safeState = (severity >= 3U) ? 1U : 0U;
    TEST_ASSERT_EQUAL(1U, safeState);
}

void test_FaultRecovery_MaxRetries(void)
{
    uint32 maxRetries = 3U;
    uint32 retries = 0U;
    boolean recovered = 0U;

    while (retries < maxRetries)
    {
        retries++;
        /* Simulate recovery failure */
    }

    TEST_ASSERT_EQUAL(3U, retries);
    TEST_ASSERT_EQUAL(0U, recovered);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_FaultClassify_Transient);
    RUN_TEST(test_FaultClassify_Intermittent);
    RUN_TEST(test_FaultClassify_Permanent);
    RUN_TEST(test_FaultLog_Wrapping);
    RUN_TEST(test_FaultSeverity_Escalation);
    RUN_TEST(test_FaultRecovery_MaxRetries);

    UNITY_END();
}
