/*
 * Copyright (c) 2019, Arm Limited and affiliates.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if !DEVICE_I2C
#error [NOT_SUPPORTED] I2C not supported for this target
#endif

#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"
//#include "i2c_test.h"

#include "mbed.h"
#include "i2c_api.h"
#include "hal/pinmap.h"
#include <vector>


using namespace utest::v1;


#define PERIPHERAL_NON (-1)
struct I2CPins {
    int peripheral;
    PinName sda, scl;
    bool test_success;

    I2CPins(int peripheral = PERIPHERAL_NON, PinName sda = NC, PinName scl = NC): peripheral(peripheral), sda(sda), scl(scl), test_success(false)
    {}

    bool empty()
    {
        return (peripheral == PERIPHERAL_NON) && (sda == NC) && (scl == NC);
    }

    friend bool operator== (const I2CPins &pins1, const I2CPins &pins2);
};

bool operator== (const I2CPins &pins1, const I2CPins &pins2)
{
    return (pins1.peripheral == pins2.peripheral) && (pins1.sda == pins2.sda) && (pins1.scl == pins2.scl);
}

const PinList *form_factor = pinmap_ff_default_pins();
const PinList *restricted = pinmap_restricted_pins();
std::vector<I2CPins> matched_pins;

void matched_pins_add(int peripheral, PinName sda, PinName scl)
{
    for (std::vector<I2CPins>::size_type i = 0; i < matched_pins.size(); i++) {
        if (matched_pins[i] == I2CPins(peripheral, sda, scl)) {
            // pins already exists on the list
            return;
        }
    }
    matched_pins.push_back(I2CPins(peripheral, sda, scl));
}

void for_each_matched_pins(bool (*test_fun)(PinName sda, PinName scl))
{
    for (std::vector<I2CPins>::size_type i = 0; i < matched_pins.size(); i++) {
        I2CPins &pins = matched_pins[i];
        if (pins.empty()) {
            break;
        }
        utest_printf("Testing pins: peripheral %d sda=%-3s scl=%-3s...",
                     pins.peripheral,
                     pinmap_ff_default_pin_to_string(pins.sda),
                     pinmap_ff_default_pin_to_string(pins.scl));
        // run test
        pins.test_success = test_fun(pins.sda, pins.scl);
        utest_printf("%s\n", pins.test_success ? "succeeded" : "failed");
    }
}

static bool find_i2c_pins(int per, PinName *sda, PinName *scl)
{
    const PinMap *maps[] = { i2c_master_sda_pinmap(), i2c_master_scl_pinmap() };
    PinName *pins[] = { sda, scl };
    return pinmap_find_peripheral_pins(form_factor, restricted, per, maps, pins, sizeof(maps) / sizeof(maps[0]));
}

template<bool (*test_fun)(PinName sda, PinName scl)>
void test_all_i2c_pins()
{
    const PinMap *maps[] = { i2c_master_sda_pinmap(), i2c_master_scl_pinmap() };
    const char *const pin_type_names[] = { "SDA", "SCL" };
    PinName pins[] = { NC, NC };
    const uint32_t pin_types = sizeof(maps) / sizeof(maps[0]);

    TEST_ASSERT_NOT_NULL(restricted);
    TEST_ASSERT_NOT_NULL(form_factor);

    // Loop through every pin type (SDA, SCL)
    for (uint32_t i = 0; i < pin_types; i++) {
        const PinMap *map = maps[i];
        const char *pin_type = pin_type_names[i];

        // Loop through each pin of a given type
        for (; map->pin != NC; map++) {

            // Clear out pins and set the one being tested
            for (int j = 0; j < pin_types; j++) {
                pins[j] = NC;
            }
            pins[i] = map->pin;
            int per = map->peripheral;

            // Only form factor pins can be tested
            if (!pinmap_list_has_pin(form_factor, pins[i])) {
                utest_printf("Skipping non form factor %s pin %s (%i)\r\n", pin_type,
                             pinmap_ff_default_pin_to_string(pins[i]), pins[i]);
                continue;
            }

            // Don't test restricted pins
            if (pinmap_list_has_pin(restricted, pins[i])) {
                utest_printf("Skipping %s pin %s (%i)\r\n", pin_type,
                             pinmap_ff_default_pin_to_string(pins[i]), pins[i]);
                continue;
            }

            if (!find_i2c_pins(per, &pins[0], &pins[1])) {
                utest_printf("Could not find a I2C pins to test %s pin %s (%i)\r\n",
                             pin_type, pinmap_ff_default_pin_to_string(pins[i]), pins[i]);
                continue;
            }
            TEST_ASSERT_NOT_EQUAL(NC, pins[0]);
            TEST_ASSERT_NOT_EQUAL(NC, pins[1]);

            matched_pins_add(per, pins[0], pins[1]);
        }
    }
    for_each_matched_pins(test_fun);
}

bool i2c_init_free(PinName sda, PinName scl)
{
    i2c_t obj = {};
    i2c_init(&obj, sda, scl);
    return true;
}


Case cases[] = {
    Case("i2c - init/free test", test_all_i2c_pins<i2c_init_free>)
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(15, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);

int main()
{
    Harness::run(specification);
}
