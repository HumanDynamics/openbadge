//
// Created by Andrew Bartow on 1/11/17.
//
#include "gtest/gtest.h"

void app_error_handler(uint32_t error_code, uint32_t line_num, const uint8_t * p_file_name) {
    printf("APP ERROR: %u @ %s:%u", error_code, p_file_name, line_num);
    EXPECT_FALSE(true);
}