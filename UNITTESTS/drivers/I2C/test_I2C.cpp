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

#include "gtest/gtest.h"
#include "drivers/I2C.h"

class TestI2C : public testing::Test {
protected:
	const int SLAVE_ADDRESS = 0x55;

    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestI2C, test_I2C_create)
{
	{
		mbed::I2C i2c(I2C_SDA, I2C_SCL);
	}

	{
		mbed::I2C i2c(I2C_SDA, I2C_SCL);
	}

	{
		mbed::I2C i2c(I2C_SDA, I2C_SCL);
	}

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_frequency_set)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	i2c.frequency(1000); // 1kHz
	i2c.frequency(100000); // 100kHz

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_read)
{
	int SLAVE_ADDRESS = 123;
	char data[16];
	int length = 16;

	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	int ret = i2c.read(SLAVE_ADDRESS, data, length);

	EXPECT_EQ(ret, 0);
}

TEST_F(TestI2C, test_I2C_read_single_byte)
{
	const int NO_ACK = 0;
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	int ret = i2c.read(NO_ACK);

	EXPECT_EQ(ret, 0x55);
}

TEST_F(TestI2C, test_I2C_write)
{
	int length = 16;
	char data[16];
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	int ret = i2c.write(SLAVE_ADDRESS, data, length);

	EXPECT_EQ(ret, 0);
}

TEST_F(TestI2C, test_I2C_write_single_byte)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	int ret = i2c.write(123);

	EXPECT_EQ(ret, 1);
}

TEST_F(TestI2C, test_I2C_start)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	i2c.start();

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_stop)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	i2c.stop();

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_lock_unlock)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	i2c.lock();
	i2c.unlock();

    EXPECT_TRUE(true);
}

static void transfer_callback(int)
{
}

TEST_F(TestI2C, test_I2C_async_transfer)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);
	int tx_length = 16;
	char tx_buffer[16];
	int rx_length = 16;
	char rx_buffer[16];
	int ret;

	ret = i2c.transfer(SLAVE_ADDRESS, tx_buffer, tx_length, rx_buffer, rx_length, transfer_callback);
	EXPECT_EQ(ret, 0);
	i2c.abort_transfer();

	ret = i2c.transfer(SLAVE_ADDRESS, tx_buffer, tx_length, NULL, 0, transfer_callback);
	EXPECT_EQ(ret, 0);
	i2c.abort_transfer();

	ret = i2c.transfer(SLAVE_ADDRESS, NULL, 0, rx_buffer, rx_length, transfer_callback);
	EXPECT_EQ(ret, 0);
	i2c.abort_transfer();

	EXPECT_TRUE(true);
}
