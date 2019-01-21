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
#ifndef Test_USBMSD_H
#define Test_USBMSD_H

#include "USBMSD.h"

class TestUSBMSD: public USBMSD {
public:
    TestUSBMSD(BlockDevice *bd, bool connect_blocking = true, uint16_t vendor_id = 0x0703, uint16_t product_id = 0x0104, uint16_t product_release = 0x0001)
        : USBMSD(bd, connect_blocking, vendor_id, product_id, product_release)
    {

    }

    virtual ~TestUSBMSD()
    {

    }

    uint32_t get_read_counter()
    {
        return read_counter;
    }

    uint32_t get_program_counter()
    {
        return program_counter;
    }

    void reset_counters()
    {
        read_counter = program_counter = erase_counter = 0;
    }

    static volatile uint32_t read_counter, program_counter, erase_counter;

protected:
    virtual int disk_read(uint8_t *data, uint64_t block, uint8_t count)
    {
        read_counter++;
        return USBMSD::disk_read(data, block, count);
    }

    virtual int disk_write(const uint8_t *data, uint64_t block, uint8_t count)
    {
        erase_counter++;
        program_counter++;

        return USBMSD::disk_write(data, block, count);
    }
};

volatile uint32_t TestUSBMSD::read_counter = 0;
volatile uint32_t TestUSBMSD::program_counter = 0;
volatile uint32_t TestUSBMSD::erase_counter = 0;

#endif // Test_USBMSD_H
