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

	{
		mbed::I2C i2c(NC, NC);
	}

	{
		mbed::I2C i2c((PinName)1234, (PinName)5678);
	}

	{
		mbed::I2C i2c(I2C_SDA, I2C_SCL);
	}

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_frequency_set)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);

	i2c.frequency(0);
	i2c.frequency(1);
	i2c.frequency(1000); // 1kHz
	i2c.frequency(1000000); // 1MHz

    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_read)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_read_single_byte)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_write)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_write_single_byte)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_start)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_stop)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_lock)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_unlock)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}

TEST_F(TestI2C, test_I2C_async_transfer)
{
	mbed::I2C i2c(I2C_SDA, I2C_SCL);


    EXPECT_TRUE(true);
}
