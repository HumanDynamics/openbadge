cmake_minimum_required(VERSION 3.6)
project(OpenBadge)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    firmware/nRF_badge/data_collector/config/nrf_drv_config.h
    firmware/nRF_badge/data_collector/config/pstorage_platform.h
    firmware/nRF_badge/data_collector/incl/analog.c
    firmware/nRF_badge/data_collector/incl/analog.h
    firmware/nRF_badge/data_collector/incl/badge_03.h
    firmware/nRF_badge/data_collector/incl/badge_03v2_dynastream.h
    firmware/nRF_badge/data_collector/incl/badge_03v2_rigado.h
    firmware/nRF_badge/data_collector/incl/badge_03v4.h
    firmware/nRF_badge/data_collector/incl/battery.c
    firmware/nRF_badge/data_collector/incl/battery.h
    firmware/nRF_badge/data_collector/incl/ble_setup.c
    firmware/nRF_badge/data_collector/incl/ble_setup.h
    firmware/nRF_badge/data_collector/incl/collector.c
    firmware/nRF_badge/data_collector/incl/collector.h
    firmware/nRF_badge/data_collector/incl/custom_board.h
    firmware/nRF_badge/data_collector/incl/debug_log.c
    firmware/nRF_badge/data_collector/incl/debug_log.h
    firmware/nRF_badge/data_collector/incl/ext_eeprom.c
    firmware/nRF_badge/data_collector/incl/ext_eeprom.h
    firmware/nRF_badge/data_collector/incl/rtc_timing.c
    firmware/nRF_badge/data_collector/incl/rtc_timing.h
    firmware/nRF_badge/data_collector/incl/scanner.c
    firmware/nRF_badge/data_collector/incl/scanner.h
    firmware/nRF_badge/data_collector/incl/self_test.c
    firmware/nRF_badge/data_collector/incl/self_test.h
    firmware/nRF_badge/data_collector/incl/sender.c
    firmware/nRF_badge/data_collector/incl/sender.h
    firmware/nRF_badge/data_collector/incl/storer.c
    firmware/nRF_badge/data_collector/incl/storer.h
    firmware/nRF_badge/data_collector/incl/uart_commands.c
    firmware/nRF_badge/data_collector/incl/uart_commands.h
    firmware/nRF_badge/data_collector/main.c)

include_directories(
        firmware/nRF_badge/data_collector/config
        firmware/nRF_badge/data_collector/incl
        ~/nrf_sdk/components/device
        ~/nrf_sdk/components/toolchain
        ~/nrf_sdk/components/toolchain/gcc
        ~/nrf_sdk/components/softdevice/s130/headers
        ~/nrf_sdk/components/softdevice/common/softdevice_handler
        ~/nrf_sdk/components/libraries/crc16/
        ~/nrf_sdk/components/libraries/util
        ~/nrf_sdk/components/libraries/fifo
        ~/nrf_sdk/components/libraries/scheduler
        ~/nrf_sdk/components/libraries/trace
        ~/nrf_sdk/components/libraries/timer
        ~/nrf_sdk/components/drivers_nrf/common
        ~/nrf_sdk/components/drivers_nrf/rtc
        ~/nrf_sdk/components/drivers_nrf/hal
        ~/nrf_sdk/components/drivers_nrf/uart
        ~/nrf_sdk/components/drivers_nrf/config
        ~/nrf_sdk/components/drivers_nrf/gpiote
        ~/nrf_sdk/components/drivers_nrf/ble_flash
        ~/nrf_sdk/components/drivers_nrf/spi_master
        ~/nrf_sdk/components/drivers_nrf/pstorage
        ~/nrf_sdk/components/ble/common
        ~/nrf_sdk/components/ble/ble_advertising
        ~/nrf_sdk/components/ble/ble_services/ble_bas
        ~/nrf_sdk/components/ble/ble_services/ble_nus
        ~/nrf_sdk/examples/bsp
)

add_executable(OpenBadge ${SOURCE_FILES})
