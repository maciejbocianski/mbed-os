
####################
# UNIT TESTS
####################

add_compile_definitions(MBED_CONF_PLATFORM_CTHUNK_COUNT_MAX=8)
add_compile_definitions(DEVICE_I2C=1)
add_compile_definitions(DEVICE_I2CSLAVE=1)
#add_compile_definitions(DEVICE_I2C_ASYNCH=1)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")


set(unittest-sources
#  ../platform/CThunkBase.cpp
  ../drivers/I2CSlave.cpp
)

set(unittest-test-sources
  drivers/I2CSlave/test_I2CSlave.cpp
  stubs/i2c_api_stub.c
#  stubs/mbed_critical_stub.c
#  stubs/mbed_error_stub.c
#  stubs/mbed_wait_api_stub.cpp
#  stubs/gpio_api_stub.c
#  stubs/mbed_assert_stub.c
)