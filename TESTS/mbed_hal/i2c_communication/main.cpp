/* mbed Microcontroller Library
 * Copyright (c) 2018-2018 ARM Limited
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

#if DEVICE_I2C_MASTER
#include "utest/utest.h"
#include "unity/unity.h"
#include "greentea-client/test_env.h"
//#include "i2c_test.h"
#endif

#include "mbed.h"
#include "i2c_api.h"

#define DEVICE_AS_MASTER   false
#define DEVICE_AS_SLAVE    true

#define SLAVE_ADDRESS 123

#define DEFAULT_FREQUENCY 1000

#if DEVICE_I2C_MASTER
#include "master_main.h"
#else
#include "slave_main.h"
#endif


int main()
{
#if DEVICE_I2C_MASTER
    master_main();
#else
    slave_main();
#endif
}
