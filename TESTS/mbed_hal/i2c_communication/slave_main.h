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


const char* status_str[4] = {"NOT_ADDRESSED", "READ", "BROADCAST", "WRITE"};

template<uint32_t data_size>
void test_i2c_transfer()
{
    i2c_t obj;

    uint8_t data[data_size] = { 0 };
    //printf("\r\n[test_i2c_transfer] start...\r\n");

    i2c_init(&obj, I2C_SDA, I2C_SCL, DEVICE_AS_SLAVE);
    //printf("[test_i2c_transfer] init OK\r\n");
    i2c_frequency(&obj, DEFAULT_FREQUENCY);

    i2c_slave_address(&obj, SLAVE_ADDRESS);
    //printf("[test_i2c_transfer] set address %u OK\r\n", SLAVE_ADDRESS);

    i2c_slave_status old_status = i2c_slave_receive(&obj);
    //printf("[test_i2c_transfer] status %s\r\n", status_str[old_status]);

#if 0
    while(true) {
        i2c_slave_status status = i2c_slave_receive(&obj);
        if(status != old_status) {
            printf("[test_i2c_transfer] status changed %s -> %s\r\n", status_str[old_status], status_str[status]);
            old_status = status;
        }
    }
#endif


    //printf("[test_i2c_transfer] start read...");
    int ret = i2c_read(&obj, 0, data, data_size, false);
    //printf("done\r\n");

    printf("[slave test_i2c_transfer] read: %d\r\n", ret);
    //for (int i = 0; i < data_size; i++)
    //   printf("[test_i2c_transfer] read[%d]: %X\r\n", i, data[i]);

//    for (int i = 0; i < data_size; i++)
//        data[i] *= 2;

    ret = i2c_write(&obj, 0, data, data_size, true);

    printf("[slave test_i2c_transfer] written: %d\r\n", ret);

    i2c_free(&obj);

    //printf("[test_i2c_transfer] done...\r\n\r\n");
}


void slave_main()
{
    printf(">>> slave start\r\n");
    test_i2c_transfer<1>();
    test_i2c_transfer<2>();
    test_i2c_transfer<11>();
    printf("<<< slave end\r\n");
}
