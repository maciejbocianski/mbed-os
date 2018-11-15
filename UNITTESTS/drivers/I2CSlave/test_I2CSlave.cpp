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
#include "drivers/I2CSlave.h"

#define I2C_SLAVE_ADDRESS(adrress)

class TestI2CSlave : public testing::Test {
protected:
	const int SLAVE_ADDRESS = 0x55;


    virtual void SetUp()
    {
    }

    virtual void TearDown()
    {
    }
};

TEST_F(TestI2CSlave, test_I2CSlave_create)
{
	{
		mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	}

	{
		mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	}

	{
		mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	}

	EXPECT_TRUE(true);
}

TEST_F(TestI2CSlave, test_I2CSlave_set_frequency)
{
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);

	i2c.frequency(1000); // 1kHz
	i2c.frequency(100000); // 100kHz

    EXPECT_TRUE(true);
}

TEST_F(TestI2CSlave, test_I2CSlave_set_address)
{
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);

	i2c.address(SLAVE_ADDRESS);

    EXPECT_TRUE(true);
}

TEST_F(TestI2CSlave, test_I2CSlave_receive)
{
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	int status = i2c.receive();

	EXPECT_EQ(status, mbed::I2CSlave::NoData);
}

TEST_F(TestI2CSlave, test_I2CSlave_read)
{
	char data[16];
	int length = 16;

	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	int ret = i2c.read(data, length);

	EXPECT_EQ(ret, 0);
}

TEST_F(TestI2CSlave, test_I2CSlave_read_single_byte)
{
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	int ret = i2c.read();

	EXPECT_EQ(ret, 0x55);
}

TEST_F(TestI2CSlave, test_I2CSlave_write)
{
	char data[16];
	int length = 16;

	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	int ret = i2c.write(data, length);

	EXPECT_EQ(ret, 0);
}

TEST_F(TestI2CSlave, test_I2CSlave_write_single_byte)
{
	int byte = 0xFF && 123;
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	int ret = i2c.write(byte);

	EXPECT_EQ(ret, 1);
}

TEST_F(TestI2CSlave, test_I2CSlave_stop)
{
	mbed::I2CSlave i2c(I2C_SDA, I2C_SCL);
	i2c.address(SLAVE_ADDRESS);

	i2c.stop();

	EXPECT_TRUE(true);
}
