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

//#include "mbed_assert.h"
#include <stdint.h>
#include "i2c_api.h"


//#include "cmsis.h"
//#include "pinmap.h"
//#include "fsl_i2c.h"
//#include "fsl_port.h"
//#include "peripheral_clock_defines.h"
//#include "PeripheralPins.h"


void i2c_init(i2c_t *obj, PinName sda, PinName scl)
{

}

int i2c_start(i2c_t *obj)
{
    return 0;
}

int i2c_stop(i2c_t *obj)
{
    return 0;
}

void i2c_frequency(i2c_t *obj, int hz)
{

}

int i2c_read(i2c_t *obj, int address, char *data, int length, int stop)
{
    return length;
}

int i2c_write(i2c_t *obj, int address, const char *data, int length, int stop)
{
    return length;
}

void i2c_reset(i2c_t *obj)
{
    i2c_stop(obj);
}

int i2c_byte_read(i2c_t *obj, int last)
{
    return 0;
}

int i2c_byte_write(i2c_t *obj, int data)
{
    return 0;
}


void i2c_slave_mode(i2c_t *obj, int enable_slave)
{

}

int i2c_slave_receive(i2c_t *obj)
{
    return 0;
}

int i2c_slave_read(i2c_t *obj, char *data, int length)
{
    return length;
}

int i2c_slave_write(i2c_t *obj, const char *data, int length)
{
    return length;
}

void i2c_slave_address(i2c_t *obj, int idx, uint32_t address, uint32_t mask)
{
}

void i2c_transfer_asynch(i2c_t *obj, const void *tx, size_t tx_length, void *rx, size_t rx_length, uint32_t address, uint32_t stop, uint32_t handler, uint32_t event, DMAUsage hint)
{
}


uint32_t i2c_irq_handler_asynch(i2c_t *obj)
{
}


uint8_t i2c_active(i2c_t *obj)
{
}


void i2c_abort_asynch(i2c_t *obj)
{
}
