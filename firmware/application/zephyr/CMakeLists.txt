# SPDX-License-Identifier: Apache-2.0

set (BOARD_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set (SHIELD beirdo_qwiic_obd_v1)

message(STATUS "${BOARD_ROOT}")

cmake_minimum_required(VERSION 3.13.1)
include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(QWIIC_OBD)

target_sources(app PRIVATE ../src/main.c)
