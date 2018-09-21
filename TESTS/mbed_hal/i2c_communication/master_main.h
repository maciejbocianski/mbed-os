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

using namespace utest::v1;

DigitalOut test_pin(D2);

void test_pin_init()
{
    test_pin.write(1);
}

void test_pin_set(int v)
{
    test_pin.write(v);
}

void test_pin_toggle()
{
    int cv = test_pin.read();
    test_pin.write(cv == 0 ? 1 : 0);
    test_pin.write(cv == 0 ? 0 : 1);
}


template<uint32_t data_size>
void test_i2c_transfer()
{
    i2c_t obj;

    uint8_t tx_buf[data_size];
    uint8_t rx_buf[data_size];

    srand(ticker_read(get_us_ticker_data()));
    for (uint32_t i = 0; i < data_size; i++) {
        tx_buf[i] = (uint8_t)(rand() % 100);
        rx_buf[0] = 0;
    }

    test_pin_set(0);
    i2c_init(&obj, I2C_SDA, I2C_SCL, DEVICE_AS_MASTER);
    i2c_frequency(&obj, DEFAULT_FREQUENCY);

    i2c_start(&obj);
    test_pin_toggle();
    int ret = i2c_write(&obj, SLAVE_ADDRESS, tx_buf, data_size, true);
    test_pin_toggle();
    i2c_stop(&obj);
    printf("[master test_i2c_transfer] i2c_write ret: %d\r\n", ret);

    i2c_start(&obj);
    test_pin_toggle();
    ret = i2c_read(&obj, SLAVE_ADDRESS, rx_buf, data_size, false);
    test_pin_toggle();
    i2c_stop(&obj);
    printf("[master test_i2c_transfer] i2c_read ret: %d\r\n", ret);
    for (int i = 0; i < data_size; i++) {
        if (rx_buf[i] != tx_buf[i]) {
            printf("transfer failed\r\n");
            TEST_ASSERT_EQUAL_INT8(tx_buf[i], rx_buf[i]);
        }
        //printf("[test_i2c_transfer] read[%d]: %X\r\n", i, rx_buf[i]);
    }


    i2c_free(&obj);

    //printf("\r\n[test_i2c_transfer] done...\r\n");
}


Case cases[] = {
    Case("test_i2c_transfer 1", test_i2c_transfer<1>),
    Case("test_i2c_transfer 2", test_i2c_transfer<2>),
    Case("test_i2c_transfer 11", test_i2c_transfer<11>),
};

utest::v1::status_t greentea_test_setup(const size_t number_of_cases)
{
    GREENTEA_SETUP(100, "default_auto");
    return greentea_test_setup_handler(number_of_cases);
}

Specification specification(greentea_test_setup, cases, greentea_test_teardown_handler);


void master_main()
{
    test_pin_init();
    Harness::run(specification);
}
