/*
 * Copyright (c) 2016-2017, ARM Limited, All Rights Reserved
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "cmsis.h"
#include "greentea-client/test_env.h"
#include "utest/utest.h"
#include "unity/unity.h"

using utest::v1::Case;

static const int test_timeout = 10;


/** Test memory layout

    Given the Mbed OS memory model
    When program is started
    Then the memory layout comply with the model

    @note for more details about Mbed OS memory model
    see https://os.mbed.com/docs/latest/reference/memory.html

 */
#if !defined(TARGET_NUVOTON) // NUVOTON don't comply with Mbed OS memory model it uses two region model
                            //  (heap and stack are two distinct regions).
void test_memory_layout(void)
{
    uint32_t psp = __get_PSP();
    uint32_t msp = __get_MSP();
    uint8_t *heap = (uint8_t*) malloc(1);

    // test if Scheduler/ISR stack is over Heap
    TEST_ASSERT_TRUE_MESSAGE(msp > (uint32_t) heap, "ISR stack / Heap placement error");
    // test if Heap is over Main stack
    TEST_ASSERT_TRUE_MESSAGE((uint32_t) heap > psp, "Heap / Main stack placement error");

    free(heap);
}
#endif


// Test cases
Case cases[] = {
#if !defined(TARGET_NUVOTON)
    Case("Test memory layout", test_memory_layout),
#endif
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(test_timeout, "default_auto");
    return utest::v1::greentea_test_setup_handler(number_of_cases);
}

utest::v1::Specification specification(greentea_test_setup, cases);

int main()
{
    return !utest::v1::Harness::run(specification);
}
