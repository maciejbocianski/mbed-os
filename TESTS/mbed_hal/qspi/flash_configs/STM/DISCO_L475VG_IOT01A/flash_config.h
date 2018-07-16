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
#ifndef MBED_QSPI_FLASH_CONFIG_H
#define MBED_QSPI_FLASH_CONFIG_H

#include "../../MX25R6435F_config.h"

// TODO: remove when fixed
// when perform 4IO write, when memory indicates write finish (changing WIP bit in status register)
// but actually write is still in progress and we have to wait a bit more before reading
#define STM_DISCO_L475VG_IOT01A_WRITE_4IO_BUG_WORKAROUND

#endif // MBED_QSPI_FLASH_CONFIG_H
