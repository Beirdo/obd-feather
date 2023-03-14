# SPDX-License-Identifier: Apache-2.0

set (BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set (SHIELD beirdo_obd_feather_v1)

message(STATUS "${BOARD_ROOT}")

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(OBD_FEATHER)

target_sources(app PRIVATE ../src/main.c)
target_sources(app PRIVATE ../src/display.c)
target_sources(app PRIVATE ../src/fonts/font5x7.c)
target_sources(app PRIVATE ../src/fonts/font8x8.c)
target_sources(app PRIVATE ../src/fonts/font10x16.c)
target_sources(app PRIVATE ../src/fonts/font15x24.c)
target_sources(app PRIVATE ../src/fonts/font20x32.c)
target_sources(app PRIVATE ../src/can.c)
target_sources(app PRIVATE ../src/gpio_map.c)

target_include_directories(app PRIVATE ../include)
