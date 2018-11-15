/*
 * Copyright (c) 2018, Arm Limited and affiliates
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

#include <string.h>
#include <stdint.h>
#include "i2c_api.h"


void i2c_get_capabilities(i2c_capabilities_t *capabilities)
{
	capabilities->minimum_frequency = 1000; // 1kHz
	capabilities->maximum_frequency = 100000; // 100kHz
	capabilities->supports_slave_mode = true;
	capabilities->supports_10bit_addressing = true;
	capabilities->supports_multi_master = true;
	capabilities->supports_clock_stretching = true;
}


void i2c_init(i2c_t *obj, PinName sda, PinName scl, bool is_slave)
{
}

void i2c_free(i2c_t *obj)
{
}

uint32_t i2c_frequency(i2c_t *obj, uint32_t hz)
{
	return hz;
}

void i2c_timeout(i2c_t *obj, uint32_t timeout)
{
}

bool i2c_start(i2c_t *obj)
{
    return true;
}

bool i2c_stop(i2c_t *obj)
{
    return true;
}

int32_t i2c_write(i2c_t *obj, uint16_t address, const void *data, uint32_t length, bool stop)
{
    return (int32_t)length;
}

int32_t i2c_read(i2c_t *obj, uint16_t address, void *data, uint32_t length, bool last)
{
	const int FILL_PATTERN = 0x55;
	memset(data, FILL_PATTERN, length);
    return (int32_t)length;
}

i2c_slave_status_t i2c_slave_status(i2c_t *obj)
{
	return NO_ADDRESS;
}

void i2c_slave_address(i2c_t *obj, uint16_t address)
{
}

void i2c_transfer_async(i2c_t *obj, const void *tx, uint32_t tx_length,
                        void *rx, uint32_t rx_length, uint16_t address,
                        bool stop, i2c_async_handler_f handler, void *ctx)
{
}

void i2c_abort_async(i2c_t *obj)
{
}
