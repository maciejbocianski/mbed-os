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

/** \addtogroup hal_i2c_tests
 *  @{
 */
#ifndef MBED_I2C_TEST_H
#define MBED_I2C_TEST_H


#if DEVICE_I2C

/** Test that i2c_get_capabilities fills given i2c_capabilities_t instance with capabilities.
 *
 *  Given is platform with I2C support.
 *  When i2c_get_capabilities is called.
 *  Then the function fills given i2c_capabilities_t structure with
 *      capabilities, and all values are correct.
 *
 */
void test_i2c_capabilities();

/** Test that i2c_init successfully initializes and i2c_free successfully frees i2c interface.
 *
 *  Given is platform with I2C support.
 *  When i2c_init and then i2c_free is called on the same i2c object multiple times.
 *  Then both init and free operation are successfully performed for both slave and master mode.
 *
 */
void test_i2c_init_free();

/** Test that i2c_frequency successfully sets given frequency.
 *
 *  Given is platform with I2C support.
 *  When call i2c_frequency with given frequency.
 *  Then the frequency is successfully set.
 *
 */
void test_i2c_frequency();

/** Test that i2c_timeout successfully sets given timeout.
 *
 *  Given is platform with I2C support.
 *  When call i2c_timeout with given timeout.
 *  Then the timeout is successfully set.
 *
 */
void test_i2c_timeout();

/** Test that i2c_start/i2c_stop successfully sends start/stop signal to the bus
 *
 *  Given is platform with I2C support.
 *  When i2c_start or i2c_stop is called.
 *  Then both succeed.
 *
 */
void test_i2c_start_stop();

/** Test i2c_read/i2c_write calls for non-existent slave address.
 *
 *  Given is platform with I2C support.
 *  When call i2c_read or i2c_write for non-existent slave address.
 *  Then I2C_ERROR_NO_SLAVE is returned.
 *
 */
void test_i2c_read_write();

#if DEVICE_I2CSLAVE
/** Test that i2c_slave_address successfully sets given address.
 *
 *  Given is platform with I2C support.
 *  When call i2c_slave_address with given slave address.
 *  Then the address is successfully set.
 *
 */
void test_i2c_slave_address();

/** Test that i2c_slave_address successfully sets given address.
 *
 *  Given is platform with I2C support.
 *  When call i2c_slave_address with given slave address.
 *  Then the address is successfully set.
 *
 */
void test_i2c_slave_receive();
#endif

#if DEVICE_I2C_ASYNCH
/** Test i2c_transfer_async call for non-existent slave address.
 *
 *  Given is platform with I2C support.
 *  When call i2c_transfer_async for non-existent slave address.
 *  Then ...
 *
 */
void test_i2c_transfer_async();
#endif

#endif

#endif

/** @}*/
