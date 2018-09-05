
/** \addtogroup hal */
/** @{*/
/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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
#ifndef MBED_I2C_API_H
#define MBED_I2C_API_H

#include "device.h"
#include "hal/buffer.h"

#if DEVICE_I2C_ASYNCH
#include "hal/dma_api.h"
#endif

#if DEVICE_I2C

/**
 * @defgroup hal_I2CEvents I2C Events Macros
 *
 * @{
 */
#define I2C_EVENT_ERROR               (1 << 1)
#define I2C_EVENT_ERROR_NO_SLAVE      (1 << 2)
#define I2C_EVENT_TRANSFER_COMPLETE   (1 << 3)
#define I2C_EVENT_TRANSFER_EARLY_NACK (1 << 4)
#define I2C_EVENT_ALL                                                          \
  (I2C_EVENT_ERROR | I2C_EVENT_TRANSFER_COMPLETE | I2C_EVENT_ERROR_NO_SLAVE |  \
   I2C_EVENT_TRANSFER_EARLY_NACK)

/**@}*/

#if DEVICE_I2C_ASYNCH
/** Asynch I2C HAL structure
 */
typedef struct {
  struct i2c_s    i2c;     /**< Target specific I2C structure */
  struct buffer_s tx_buff; /**< Tx buffer */
  struct buffer_s rx_buff; /**< Rx buffer */
} i2c_t;

#else
/** Non-asynch I2C HAL structure
 */
typedef struct i2c_s i2c_t;

#endif

enum {
    I2C_ERROR_NO_SLAVE = -1,
    I2C_ERROR_BUS_BUSY = -2
};

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup hal_GeneralI2C I2C Configuration Functions
 * @{
 */

/** Initialize the I2C peripheral. It sets the default parameters for I2C
 *  peripheral, and configures its pins.
 *
 *  @param obj       The I2C object
 *  @param sda       The sda pin
 *  @param scl       The scl pin
 *  @param is_slave  Select whether the peripheral is initialised as master or
 *                   slave.
 */
void i2c_init(i2c_t *obj, PinName sda, PinName scl, bool is_slave);

/** Configure the frequency in Hz the I2C peripheral should operate at.
 *
 *  @param obj        The I2C object
 *  @param frequency  Frequency in Hz
 */
void i2c_frequency(i2c_t *obj, uint32_t frequency);

/** Configure the timeout duration in milliseconds the I2C peripheral should
 * allow the slave peripheral to stretch the clock for before timing out.
 *
 *  @param obj        The I2C object
 *  @param timeout    Clock stretching timeout in milliseconds.
 */
void i2c_timeout(i2c_t *obj, uint32_t timeout);

/** Send START command
 *
 *  @param obj The I2C object
 *  @returns   True if slave responds with ACK, false otherwise.
 */
bool i2c_start(i2c_t *obj);

/** Send STOP command
 *
 *  @param obj The I2C object
 *  @returns   True if slave responds with ACK, false otherwise.
 */
bool i2c_stop(i2c_t *obj);

/** Blocking reading data
 *
 *  @param obj     The I2C object
 *  @param address 7-bit address (last bit is 1)
 *  @param data    The buffer for receiving
 *  @param length  Number of bytes to read
 *  @param last    If true, indicates that the transfer contains the last byte
 *                 to be sent.
 *
 *  @note If the current platform supports multimaster operation the transfer
 *        will block until the peripheral can gain arbitration of the bus and
 *        complete the transfer. If the device does not support multimaster
 *        operation this function is not safe to execute when the bus is shared
 *        with another device in master mode.
 *
 *  @return
 *      zero or non-zero - Number of written bytes
 *      negative - I2C_ERROR_XXX status
 */
int i2c_read(i2c_t *obj, uint16_t address, void *data, uint32_t length,
             bool last);

/** Blocking sending data
 *
 *  @param obj     The I2C object
 *  @param address 7-bit address (last bit is 0)
 *  @param data    The buffer for sending
 *  @param length  Number of bytes to write
 *  @param stop    If true, stop will be generated after the transfer is done
 *
 *  @note If the current platform supports multimaster operation the transfer
 *        will block until the peripheral can gain arbitration of the bus and
 *        complete the transfer. If the device does not support multimaster
 *        operation this function is not safe to execute when the bus is shared
 *        with another device in master mode.
 *
 *  @return
 *      zero or non-zero - Number of written bytes
 *      negative - I2C_ERROR_XXX status
 */
int i2c_write(i2c_t *obj, uint16_t address, const void *data, uint32_t length,
              bool stop);

/**@}*/

#if DEVICE_I2CSLAVE

/**
 * \defgroup SynchI2C Synchronous I2C Hardware Abstraction Layer for slave
 * @{
 */

/** Check to see if the I2C slave has been addressed.
 *  @param obj The I2C object
 *  @return The status - i2c_slave_status indicating what mode the peripheral is
 * configured in.
 */
i2c_slave_status i2c_slave_receive(i2c_t *obj);

/** Configure I2C address.
 *
 *  @param obj     The I2C object
 *  @param address The address to be set
 */
void i2c_slave_address(i2c_t *obj, uint16_t address);

#endif

/**@}*/

#if DEVICE_I2C_ASYNCH

/**
 * \defgroup hal_AsynchI2C Asynchronous I2C Hardware Abstraction Layer
 * @{
 */

typedef void (*i2c_async_handler_f)(i2c_t *obj, void *ctx,
                                    i2c_async_event_t event);

/** Start I2C asynchronous transfer
 *
 *  @param obj       The I2C object
 *  @param tx        The transmit buffer
 *  @param tx_length The number of bytes to transmit
 *  @param rx        The receive buffer
 *  @param rx_length The number of bytes to receive
 *  @param address   The address to be set - 7bit or 9bit
 *  @param stop      If true, stop will be generated after the transfer is done
 *  @param handler   The I2C IRQ handler to be set
 *  @param hint      DMA hint usage
 */
void i2c_transfer_async(i2c_t *obj, const void *tx, uint32_t tx_length,
                        void *rx, uint32_t rx_length, uint16_t address,
                        bool stop, i2c_async_handler_f handler, DMAUsage hint);

/** Abort asynchronous transfer
 *
 *  This function does not perform any check - that should happen in upper
 * layers.
 *  @param obj The I2C object
 */
void i2c_abort_async(i2c_t *obj);

#endif

/**@}*/

#ifdef __cplusplus
}
#endif

#endif

#endif

/** @}*/
