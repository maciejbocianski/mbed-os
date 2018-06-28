/*
 * Copyright (c) 2013-2017, ARM Limited, All Rights Reserved
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
#include "greentea-client/test_env.h"
#include "utest/utest.h"
#include "unity/unity.h"


#define MAX_UINT32 4294967295

// MAX_UINT32 -10
#define BEFORE_OVERFLOW 4294967285ULL

// MAX_UINT32 + 10
#define AFTER_OVERFLOW 4294967305ULL

static us_timestamp_t present_time = 0;

using utest::v1::Case;

timestamp_t ticker_read(const ticker_data_t *const ticker)
{
    return ticker_read_us(NULL);
}

us_timestamp_t ticker_read_us(const ticker_data_t *const ticker)
{
    return present_time++;
}


void test_wait_us_32()
{
    present_time = BEFORE_OVERFLOW;

    wait_us(20);
    us_timestamp_t present_time_after_wait = present_time;

    printf("present_time_after_wait: %llu\r\n", present_time_after_wait);
    printf("expected:                %llu\r\n", AFTER_OVERFLOW + 1);
}

void test_wait_us_64()
{
    present_time = BEFORE_OVERFLOW;

    wait_us64(20);
    us_timestamp_t present_time_after_wait = present_time;

    printf("present_time_after_wait: %llu\r\n", present_time_after_wait);
    printf("expected:                %llu\r\n", AFTER_OVERFLOW + 1);
}

void test_wait_us_32_negative()
{
    present_time = BEFORE_OVERFLOW;

    wait_us(-20);
    us_timestamp_t present_time_after_wait = present_time;

    printf("present_time_after_wait: %llu\r\n", present_time_after_wait);
    printf("expected:                %llu\r\n", AFTER_OVERFLOW + 1);
}

void test_wait_us_64_negative()
{
    present_time = BEFORE_OVERFLOW;

    wait_us(-20);
    us_timestamp_t present_time_after_wait = present_time;

    printf("present_time_after_wait: %llu\r\n", present_time_after_wait);
    printf("expected:                %llu\r\n", AFTER_OVERFLOW + 1);
}

Case cases[] = {
    //Case("Test wait_us 32bit version", test_wait_us_32),
    //Case("Test wait_us 64bit version", test_wait_us_64),
    Case("Test wait_us 32bit version negative value passing", test_wait_us_32_negative),
    //Case("Test wait_us 64bit version negative value passing", test_wait_us_64_negative),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(10, "timing_drift_auto");
    return utest::v1::greentea_test_setup_handler(number_of_cases);
}

utest::v1::Specification specification(greentea_test_setup, cases);

int main()
{
    utest::v1::Harness::run(specification);
}
