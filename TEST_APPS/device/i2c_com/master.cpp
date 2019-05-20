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

#include "common.h"
#include "mbed.h"

#define I2C_MASTER_SDA MBED_CONF_APP_I2C_MASTER_SDA
#define I2C_MASTER_SCL MBED_CONF_APP_I2C_MASTER_SCL


#if DEVICE_I2C_ASYNCH
struct  async_status {
    volatile i2c_async_event_t event;
    volatile bool done;
};
#endif

#ifdef TEST_I2C_DRIVER

class I2CTest : public I2C {
public:
    I2CTest(PinName sda, PinName scl) : I2C(sda, scl)
    {

    }
#if DEVICE_I2C_ASYNCH
    async_status *transfer_status;
#endif
};

static I2CTest *i2c_obj;

static void _i2c_init()
{
    i2c_obj = new I2CTest(I2C_MASTER_SDA, I2C_MASTER_SCL);
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

static void _i2c_timeout(uint32_t timeout)
{
    i2c_obj->timeout(timeout);
}

static void _i2c_stop()
{
    i2c_obj->stop();
}


static int32_t _i2c_read(uint16_t address, void *data, uint32_t length, bool last)
{
    int ret = i2c_obj->read(address, (char *)data, length);
    if (last) {
        i2c_obj->stop();
    }
    return ret == 0 ? length : -1;
}

static int32_t _i2c_write(uint16_t address, const void *data, uint32_t length, bool stop)
{
    int ret = i2c_obj->write(address, (char *)data, length);
    if (stop) {
        i2c_obj->stop();
    }
    return ret == 0 ? length : -1;
}

#if DEVICE_I2C_ASYNCH
void _i2c_transfer_async(const void *tx, uint32_t tx_length, void *rx, uint32_t rx_length,
                         uint16_t address, bool stop, event_callback_t &callback, void *ctx)
{
    i2c_obj->transfer_status = (async_status *)ctx;
    i2c_obj->transfer_status->event.sent_bytes = tx_length;
    i2c_obj->transfer_status->event.received_bytes = rx_length;
    i2c_obj->transfer(address, (const char *)tx, tx_length, (char *)rx, rx_length, callback, I2C_EVENT_TRANSFER_COMPLETE, !stop);
}
#endif
#else // TEST_I2C_DRIVER

static i2c_t i2c_obj;

static void _i2c_init()
{
    memset(&i2c_obj, 0, sizeof(i2c_t));
    i2c_init(&i2c_obj, I2C_MASTER_SDA, I2C_MASTER_SCL, DEVICE_AS_MASTER);
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

static void _i2c_stop()
{
    i2c_stop(&i2c_obj);
}

static int32_t _i2c_read(uint16_t address, uint8_t *data, uint32_t length, bool last)
{
    return i2c_read(&i2c_obj, address, data, length, last);
}

static int32_t _i2c_write(uint16_t address, const uint8_t *data, uint32_t length, bool stop)
{
    return i2c_write(&i2c_obj, address, data, length, stop);
}
#if DEVICE_I2C_ASYNCH
void _i2c_transfer_async(const uint8_t *tx, uint32_t tx_length, uint8_t *rx, uint32_t rx_length,
                         uint16_t address, bool stop, i2c_async_handler_f handler, void *ctx)
{
    i2c_transfer_async(&i2c_obj, tx, tx_length, rx, rx_length, address, stop, handler, ctx);
}
#endif
#endif // TEST_I2C_DRIVER



bool master_init(uint32_t frequency)
{
    i2c_capabilities_t cap;
    i2c_get_capabilities(&cap);

    _i2c_init();
    uint32_t set_freq = _i2c_frequency(frequency);
    bool result = TEST_CHECK_UINT_WITHIN(cap.minimum_frequency, cap.maximum_frequency, set_freq);

    return result;
}

bool master_deinit()
{
    _i2c_free();
    return true;
}


bool master_write_read(uint32_t write_size, uint32_t read_size, uint16_t write_address, uint16_t read_address, uint32_t iterations, bool stop_each, bool stop_each_iter)
{
    bool result = true;
    uint8_t _buf[MAX_STACK_DATA];
    const uint32_t max_size = MAX(write_size, read_size);
    uint8_t *buf = max_size <= MAX_STACK_DATA ? _buf : new uint8_t[max_size];

    for (uint32_t i = 0; i < iterations && result; i++) {
        int ret;

        for (uint32_t j = 0; j < write_size; j++) {
            buf[j] = j % TEST_PATTERN_SIZE;
        }

        MASTER_PIN_TOGGLE(1);
        ret = _i2c_write(write_address, buf, write_size, stop_each);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] write count: %d\n", ret);
        I2C_DEBUG_PRINTF("[master] write data: ");
        for (uint32_t j = 0; j < write_size; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(write_size, ret);

        if (result) {
            MASTER_PIN_TOGGLE(1);
            ret = _i2c_read(read_address, buf, read_size, stop_each || stop_each_iter);
            MASTER_PIN_TOGGLE(1);
            I2C_DEBUG_PRINTF("[master] read count: %d\n", ret);
            I2C_DEBUG_PRINTF("[master] read data:    ");
            for (uint32_t j = 0; j < read_size; j++) {
                I2C_DEBUG_PRINTF("%X ", buf[j]);
            }
            I2C_DEBUG_PRINTF("\r\n");
            result = TEST_CHECK_EQUAL_INT(read_size, ret);
            for (uint32_t j = 0; j < read_size; j++) {
                if (buf[j] != (read_size - j - 1) % TEST_PATTERN_SIZE) {
                    result = TEST_CHECK_EQUAL_INT((read_size - j - 1) % TEST_PATTERN_SIZE, buf[j]);
                    break;
                }
            }
        }
    }
    if (!stop_each && !stop_each_iter) {
        _i2c_stop();
    }

    if (max_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

bool master_read(uint32_t read_size, uint32_t read_result, uint16_t address, uint32_t iterations, bool stop_each_iter)
{
    bool result = true;
    uint8_t _buf[MAX_STACK_DATA];
    uint8_t *buf = read_size <= MAX_STACK_DATA ? _buf : new uint8_t[read_size];

    for (uint32_t i = 0; i < iterations && result; i++) {
        MASTER_PIN_TOGGLE(1);
        int ret = _i2c_read(MAKE_7BIT_READ_ADDRESS(address), buf, read_size, stop_each_iter);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] read count: %d\n", ret);
        I2C_DEBUG_PRINTF("[master] read data:    ");
        for (uint32_t j = 0; j < read_size; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(read_result, ret);

        if (result) {
            for (uint32_t j = 0; j < read_result; j++) {
                const uint8_t pat = (read_size - j - 1) % TEST_PATTERN_SIZE;
                if (buf[j] != pat) {
                    result = TEST_CHECK_EQUAL_INT(pat, buf[j]);
                    break;
                }
            }
        }
    }
    if (!stop_each_iter) {
        _i2c_stop();
    }

    if (read_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

bool master_write(uint32_t write_size, uint32_t write_result, uint16_t address, uint32_t iterations, bool stop_each_iter)
{
    bool result = true;
    uint8_t _buf[MAX_STACK_DATA];
    uint8_t *buf = write_size <= MAX_STACK_DATA ? _buf : new uint8_t[write_size];

    for (uint32_t j = 0; j < write_size; j++) {
        buf[j] = j % TEST_PATTERN_SIZE;
    }

    for (uint32_t i = 0; i < iterations && result; i++) {
        MASTER_PIN_TOGGLE(1);
        int ret = _i2c_write(MAKE_7BIT_WRITE_ADDRESS(address), buf, write_size, stop_each_iter);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] write count: %d\n", ret);
        I2C_DEBUG_PRINTF("[master] written data: ");
        for (uint32_t j = 0; j < write_result; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(write_result, ret);

    }
    if (!stop_each_iter) {
        _i2c_stop();
    }

    if (write_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}


#if DEVICE_I2C_ASYNCH

#ifdef TEST_I2C_DRIVER
void async_callback_handler(int event)
{
    async_status *s = i2c_obj->transfer_status;
    s->event.error = event != I2C_EVENT_TRANSFER_COMPLETE;
    if (s->event.error) {
        i2c_obj->transfer_status->event.sent_bytes = 0;
        i2c_obj->transfer_status->event.received_bytes = 0;
    }
    s->done = true;
}
event_callback_t async_callback(async_callback_handler);
#else // TEST_I2C_DRIVER
void async_callback(i2c_t *obj, i2c_async_event_t *event, void *ctx)
{
    async_status *s = ((async_status *)ctx);
    s->event.sent_bytes = event->sent_bytes;
    s->event.received_bytes = event->received_bytes;
    s->event.error = event->error;
    s->done = true;
}
#endif // TEST_I2C_DRIVER

bool master_write_read_async(uint32_t write_size, uint32_t read_size, uint16_t address, uint32_t iterations, bool stop_each, bool stop_each_iter)
{
    async_status transmit_status = {};
    async_status receive_status = {};
    uint8_t _buf[MAX_STACK_DATA];
    const uint32_t max_size = MAX(write_size, read_size);
    uint8_t *buf = max_size <= MAX_STACK_DATA ? _buf : new uint8_t[max_size];
    bool result = true;

    for (uint32_t i = 0; i < iterations && result; i++) {
        for (uint32_t j = 0; j < write_size; j++) {
            buf[j] = j % TEST_PATTERN_SIZE;
        }
        transmit_status.done = false;
        MASTER_PIN_TOGGLE(1);
        _i2c_transfer_async(buf, write_size, NULL, 0, MAKE_7BIT_WRITE_ADDRESS(address), stop_each, async_callback, &transmit_status);
        MASTER_PIN_TOGGLE(1);
        while (!transmit_status.done);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] sent_bytes: %u\r\n", transmit_status.event.sent_bytes);
        I2C_DEBUG_PRINTF("[master] write data: ");
        for (uint32_t j = 0; j < write_size; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(write_size, transmit_status.event.sent_bytes);

        if (result) {
            receive_status.done = false;
            MASTER_PIN_TOGGLE(1);
            _i2c_transfer_async(NULL, 0, buf, read_size, MAKE_7BIT_READ_ADDRESS(address), stop_each || stop_each_iter, async_callback, &receive_status);
            MASTER_PIN_TOGGLE(1);
            while (!receive_status.done);
            MASTER_PIN_TOGGLE(1);
            I2C_DEBUG_PRINTF("[master] received_bytes: %u\r\n", receive_status.event.received_bytes);
            I2C_DEBUG_PRINTF("[master] read data: ");
            for (uint32_t j = 0; j < read_size; j++) {
                I2C_DEBUG_PRINTF("%X ", buf[j]);
            }
            I2C_DEBUG_PRINTF("\r\n");
            result = TEST_CHECK_EQUAL_INT(read_size, receive_status.event.received_bytes);
            for (uint32_t j = 0; j < read_size; j++) {
                if (buf[j] != (read_size - j - 1) % TEST_PATTERN_SIZE) {
                    result = TEST_CHECK_EQUAL_INT((read_size - j - 1) % TEST_PATTERN_SIZE, buf[j]);
                    break;
                }
            }
        }
    }
    if (!stop_each && !stop_each_iter) {
        _i2c_stop();
    }

    if (max_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

bool master_write_async(uint32_t write_size, uint32_t write_reulting_size, uint16_t address, uint32_t iterations, bool stop_each)
{
    async_status transmit_status = {};
    uint8_t _buf[MAX_STACK_DATA];
    uint8_t *buf = write_size <= MAX_STACK_DATA ? _buf : new uint8_t[write_size];
    bool result = true;

    for (uint32_t j = 0; j < write_size; j++) {
        buf[j] = j % TEST_PATTERN_SIZE;
    }

    for (uint32_t i = 0; i < iterations && result; i++) {
        transmit_status.done = false;
        MASTER_PIN_TOGGLE(1);
        _i2c_transfer_async(buf, write_size, NULL, 0, MAKE_7BIT_WRITE_ADDRESS(address), stop_each, async_callback, &transmit_status);
        MASTER_PIN_TOGGLE(1);
        while (!transmit_status.done);
        MASTER_PIN_TOGGLE(1);
        result = TEST_CHECK_EQUAL_INT(write_reulting_size, transmit_status.event.sent_bytes);
        I2C_DEBUG_PRINTF("[master] sent_bytes: %u\r\n", transmit_status.event.sent_bytes);
        I2C_DEBUG_PRINTF("[master] write data: ");
        for (uint32_t j = 0; j < write_reulting_size; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
    }
    if (!stop_each) {
        _i2c_stop();
    }

    if (write_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

bool master_read_async(uint32_t read_size, uint32_t read_resulting_size, uint16_t address, uint32_t iterations, bool stop_each)
{
    async_status receive_status = {};
    uint8_t _buf[MAX_STACK_DATA];
    uint8_t *buf = read_size <= MAX_STACK_DATA ? _buf : new uint8_t[read_size];
    bool result = true;

    for (uint32_t i = 0; i < iterations && result; i++) {
        receive_status.done = false;
        MASTER_PIN_TOGGLE(1);
        _i2c_transfer_async(NULL, 0, buf, read_size, MAKE_7BIT_READ_ADDRESS(address), stop_each, async_callback, &receive_status);
        MASTER_PIN_TOGGLE(1);
        while (!receive_status.done);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] received_bytes: %u\r\n", receive_status.event.received_bytes);
        I2C_DEBUG_PRINTF("[master] read data: ");
        for (uint32_t j = 0; j < read_resulting_size; j++) {
            I2C_DEBUG_PRINTF("%X ", buf[j]);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(read_resulting_size, receive_status.event.received_bytes);

        if (result) {
            for (uint32_t j = 0; j < read_resulting_size; j++) {
                if (buf[j] != (read_size - j - 1) % TEST_PATTERN_SIZE) {
                    result = TEST_CHECK_EQUAL_INT((read_size - j - 1) % TEST_PATTERN_SIZE, buf[j]);
                    break;
                }
            }
        }
    }
    if (!stop_each) {
        _i2c_stop();
    }

    if (read_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

bool master_transfer_async(uint32_t write_size, uint32_t read_size, uint16_t address, uint32_t iterations, bool stop_each)
{
    async_status transfer_status = {};
    uint8_t _buf[MAX_STACK_DATA];
    const uint32_t max_size = MAX(write_size, read_size);
    uint8_t *buf = max_size <= MAX_STACK_DATA ? _buf : new uint8_t[max_size];
    bool result = true;

    srand(ticker_read(get_us_ticker_data()));

    for (uint32_t i = 0; i < iterations && true; i++) {
        for (uint32_t j = 0; j < write_size; j++) {
            buf[j] = j % TEST_PATTERN_SIZE;
        }
        transfer_status.done = false;
        MASTER_PIN_TOGGLE(1);
        _i2c_transfer_async(buf, write_size, buf, read_size, MAKE_7BIT_WRITE_ADDRESS(address), stop_each, async_callback, &transfer_status);
        MASTER_PIN_TOGGLE(1);
        while (!transfer_status.done);
        MASTER_PIN_TOGGLE(1);
        I2C_DEBUG_PRINTF("[master] sent_bytes: %u\n", transfer_status.event.sent_bytes);
        I2C_DEBUG_PRINTF("[master] written data: ");
        for (uint32_t j = 0; j < write_size; j++) {
            I2C_DEBUG_PRINTF("%X ", j % TEST_PATTERN_SIZE;);
        }
        I2C_DEBUG_PRINTF("\r\n");
        result = TEST_CHECK_EQUAL_INT(write_size, transfer_status.event.sent_bytes);

        if (result) {
            I2C_DEBUG_PRINTF("[master] received_bytes: %u\n", transfer_status.event.received_bytes);
            I2C_DEBUG_PRINTF("[master] read data: ");
            for (uint32_t j = 0; j < read_size; j++) {
                I2C_DEBUG_PRINTF("%X ", (read_size - j - 1) % TEST_PATTERN_SIZE;);
            }
            I2C_DEBUG_PRINTF("\r\n");
            result = TEST_CHECK_EQUAL_INT(read_size, transfer_status.event.received_bytes);
            for (uint32_t j = 0; j < read_size; j++) {
                if (buf[j] != (read_size - j - 1) % TEST_PATTERN_SIZE) {
                    result = TEST_CHECK_EQUAL_INT((read_size - j - 1) % TEST_PATTERN_SIZE, buf[j]);
                    break;
                }
            }
        }
    }
    if (!stop_each) {
        _i2c_stop();
    }

    if (max_size > MAX_STACK_DATA) {
        delete [] buf;
    }

    return result;
}

#endif
