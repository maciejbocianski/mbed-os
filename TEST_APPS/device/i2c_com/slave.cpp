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

#if DEVICE_I2CSLAVE

#include "slave.h"
#include "common.h"

#define I2C_SLAVE_SDA MBED_CONF_APP_I2C_SLAVE_SDA
#define I2C_SLAVE_SCL MBED_CONF_APP_I2C_SLAVE_SCL

volatile bool done;


#ifdef TEST_I2C_DRIVER
static I2CSlave *i2c_obj;

static void _i2c_init()
{
    i2c_obj = new I2CSlave(I2C_SLAVE_SDA, I2C_SLAVE_SCL);
}

static void _i2c_free()
{
    delete i2c_obj;
}

static uint32_t _i2c_frequency(uint32_t frequency)
{
    i2c_obj->frequency(frequency);
    return frequency;
}

static void _i2c_slave_address(uint16_t address)
{
    i2c_obj->address(address);
}

static i2c_slave_status_t _i2c_slave_status()
{
    return (i2c_slave_status_t)i2c_obj->receive();
}

static int32_t _i2c_read(void *data, uint32_t length, bool last)
{
    int ret = i2c_obj->read((char *)data, length);
    if (last) {
        i2c_obj->stop();
    }
    return ret == 0 ? length : -1;
}

static int32_t _i2c_write(const void *data, uint32_t length, bool stop)
{
    int ret = i2c_obj->write((char *)data, length);
    if (stop) {
        i2c_obj->stop();
    }
    return ret == 0 ? length : -1;
}

#else // TEST_I2C_DRIVER

static i2c_t i2c_obj;

static void _i2c_init()
{
    memset(&i2c_obj, 0, sizeof(i2c_t));
    i2c_init(&i2c_obj, I2C_SLAVE_SDA, I2C_SLAVE_SCL, DEVICE_AS_SLAVE);
}

static void _i2c_free()
{
    i2c_free(&i2c_obj);
}

static uint32_t _i2c_frequency(uint32_t frequency)
{
    return i2c_frequency(&i2c_obj, frequency);
}

static void _i2c_timeout(uint32_t timeout)
{
    i2c_timeout(&i2c_obj, timeout);
}

static void _i2c_slave_address(uint16_t address)
{
    i2c_slave_address(&i2c_obj, address);
}

static i2c_slave_status_t _i2c_slave_status()
{
    return i2c_slave_status(&i2c_obj);
}

static int32_t _i2c_read(uint8_t *data, uint32_t length, bool last)
{
    return i2c_read(&i2c_obj, 0, data, length, last);
}

static int32_t _i2c_write(const uint8_t *data, uint32_t length, bool stop)
{
    return i2c_write(&i2c_obj, 0, data, length, stop);
}

#endif // TEST_I2C_DRIVER



bool slave_init(uint32_t frequency)
{
    i2c_capabilities_t cap;
    i2c_get_capabilities(&cap);

    _i2c_init();
    uint32_t set_freq = _i2c_frequency(frequency);
    bool result = TEST_CHECK_UINT_WITHIN(cap.minimum_frequency, cap.maximum_frequency, set_freq);

    return result;
}

bool slave_deinit()
{
    _i2c_free();
    return true;
}

void slave_finish()
{
    done = true;
}

void slave_transfer_job(TransferConfig *tc)
{
    bool result;
    const uint32_t max_size = MAX(tc->read_size, tc->write_size);
    uint8_t _data[MAX_STACK_DATA];
    uint8_t *data = max_size <= MAX_STACK_DATA ? _data : new uint8_t[max_size];
    _i2c_slave_address(tc->address);

    for (uint32_t i = 0; i < tc->iterations; i++) {
        int ret;
        result = true;
        SLAVE_PIN_TOGGLE(1);
        while (_i2c_slave_status() != WriteAddressed && !done);
        SLAVE_PIN_TOGGLE(1);
        ret = _i2c_read(data, tc->read_size, false);
        SLAVE_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[slave] read data count: %d\n", ret);
        I2C_DEBUG_PRINTF("[slave] read data: ");
        for (uint32_t j = 0; j < tc->read_size; j++) {
            I2C_DEBUG_PRINTF("%X ", data[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(tc->read_size_resulting, ret);
        for (uint32_t j = 0; j < tc->read_size; j++) {
            if (data[j] != j % TEST_PATTERN_SIZE) {
                result = TEST_CHECK_EQUAL_INT(j, data[j]);
                break;
            }
        }

        if (result) {
            for (uint32_t j = 0; j < tc->write_size; j++) {
                data[j] = (tc->write_size - j - 1) % TEST_PATTERN_SIZE;
            }
            SLAVE_PIN_TOGGLE(1);
            while (_i2c_slave_status() != ReadAddressed && !done);
            SLAVE_PIN_TOGGLE(1);
            ret = _i2c_write(data, tc->write_size, false);
            SLAVE_PIN_TOGGLE(1);
            I2C_DEBUG_PRINTF("[slave] write data count: %d\n", ret);
            result = TEST_CHECK_EQUAL_INT(tc->write_size_resulting, ret);
        }

        if (!result || done) {
            break;
        }
    }
    tc->result = result;

    if (max_size > MAX_STACK_DATA) {
        delete [] data;
    }
}

void slave_read_job(TransferConfig *tc)
{
    bool result = true;
    uint8_t _data[MAX_STACK_DATA];
    uint8_t *data = tc->read_size <= MAX_STACK_DATA ? _data : new uint8_t[tc->read_size];

    _i2c_slave_address(tc->address);

    for (uint32_t i = 0; i < tc->iterations; i++) {
        SLAVE_PIN_TOGGLE(1);
        while (_i2c_slave_status() != WriteAddressed && !done);
        SLAVE_PIN_TOGGLE(1);
        int ret = _i2c_read(data, tc->read_size, false);
        SLAVE_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[slave] read data count: %d\n", ret);
        I2C_DEBUG_PRINTF("[slave] read data: ");
        for (uint32_t j = 0; j < tc->read_size; j++) {
            I2C_DEBUG_PRINTF("%X ", data[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(tc->read_size_resulting, ret);

        if (result) {
            for (uint32_t j = 0; j < tc->read_size_resulting; j++) {
                if (data[j] != j % TEST_PATTERN_SIZE) {
                    result = TEST_CHECK_EQUAL_INT(j, data[j]);
                    break;
                }
            }
        }

        if (!result || done) {
            break;
        }
    }
    tc->result = result;

    if (tc->read_size > MAX_STACK_DATA) {
        delete [] data;
    }
}

void slave_write_job(TransferConfig *tc)
{
    bool result = true;
    uint8_t _data[MAX_STACK_DATA];
    uint8_t *data = tc->write_size <= MAX_STACK_DATA ? _data : new uint8_t[tc->write_size];

    // prepare data to send
    for (uint32_t i = 0; i < tc->write_size; i++) {
        data[i] = (tc->write_size - i - 1) % TEST_PATTERN_SIZE;
    }

    _i2c_slave_address(tc->address);

    for (uint32_t i = 0; i < tc->iterations; i++) {
        SLAVE_PIN_TOGGLE(1);
        while (_i2c_slave_status() != ReadAddressed && !done);
        SLAVE_PIN_TOGGLE(1);
        int ret = _i2c_write(data, tc->write_size, false);
        SLAVE_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[slave] write data count: %d\n", ret);
        result = TEST_CHECK_EQUAL_INT(tc->write_size_resulting, ret);

        if (!result || done) {
            break;
        }
    }
    tc->result = result;

    if (tc->read_size > MAX_STACK_DATA) {
        delete [] data;
    }
}

bool slave_transfer(uint32_t write_size, uint32_t write_size_resulting, uint32_t read_size, uint32_t read_size_resulting, uint16_t address, uint32_t iterations)
{
    TransferConfig tc = {};
    tc.write_size = write_size;
    tc.read_size = read_size;
    tc.iterations = iterations;
    tc.address = address;
    tc.write_size_resulting = write_size_resulting;
    tc.read_size_resulting = read_size_resulting;
    tc.result = false;
    done = false;

    void (*job)(TransferConfig * tc) = NULL;
    if (write_size && read_size) {
        job = slave_transfer_job;
    } else if (write_size) {
        job = slave_write_job;
    } else if (read_size) {
        job = slave_read_job;
    } else {
        return false;
    }

    // MBedOS main thread stack is 4kB (3 kB for some small RAM targets)
    // so 1.5kB for slave thread is OK
    WorkerThread<1536> slave_thread;
    slave_thread.start(callback(job, &tc));
    slave_thread.join();

    return tc.result;
}

#endif // DEVICE_I2CSLAVE
