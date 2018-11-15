
####################
# UNIT TESTS
####################

add_compile_definitions(MBED_CONF_PLATFORM_CTHUNK_COUNT_MAX=8)
add_compile_definitions(DEVICE_I2C=1)
add_compile_definitions(DEVICE_I2C_ASYNCH=1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")


set(unittest-sources
  ../platform/CThunkBase.cpp
  ../drivers/I2C.cpp
)

set(unittest-test-sources
  drivers/I2C/test_I2C.cpp
  stubs/i2c_api_stub.c
  stubs/mbed_critical_stub.c
  stubs/mbed_error_stub.c
  stubs/mbed_wait_api_stub.cpp
  stubs/gpio_api_stub.c
  stubs/mbed_assert_stub.c
)