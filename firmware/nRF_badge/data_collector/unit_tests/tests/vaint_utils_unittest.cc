#include "gtest/gtest.h"
#include "vaint_utils.h"
#include "calc_macros.h"

#define BITS_PER_BYTE      8
#define UINT32_LEN_BITS    (BITS_PER_BYTE * sizeof(uint32_t))
#define MAX_VAINT_LEN_BITS (UINT32_LEN_BITS + CEIL(UINT32_LEN_BITS, 3))
#define MAX_VAINT_LEN      CEIL(MAX_VAINT_LEN_BITS, BITS_PER_BYTE)

static void test_conversions_of_value(uint32_t value, uint8_t * vaint, uint32_t vaint_len_bits, bool start_at_half_byte) {
    uint8_t decoded_vaint_len;
    EXPECT_EQ(value, VaintUtil_DecodeVaint(vaint, start_at_half_byte, &decoded_vaint_len));
    EXPECT_EQ(vaint_len_bits, decoded_vaint_len);

    uint8_t result[MAX_VAINT_LEN];
    memset(result, 0, sizeof(result));
    uint8_t result_len_bits;
    VaintUtil_EncodeVaint(value, start_at_half_byte, result, &result_len_bits);
    EXPECT_EQ(vaint_len_bits, result_len_bits);
    size_t vaint_len_bytes = (size_t) CEIL(result_len_bits, BITS_PER_BYTE);

    EXPECT_EQ(0, memcmp(vaint, result, vaint_len_bytes));

    if (memcmp(vaint, result, vaint_len_bytes) != 0) {
        printf("Differing values in arr of len %lu: ", vaint_len_bytes);
        for (int i = 0; i < vaint_len_bytes; i++) {
            printf("(%02X, %02X), ", vaint[i], result[i]);
        }
        printf("\r\n");
    }
}

TEST(VaintUtilsTest, TestSmall) {
    // Vaint Zero: 0b1000 = 0x80
    uint8_t vaint_zero = 0x80;
    test_conversions_of_value(0, &vaint_zero, 4, false);

    // Vaint Three: 0b1011 = 0xB0
    uint8_t vaint_three = 0xB0;
    test_conversions_of_value(3, &vaint_three, 4, false);
}

TEST(VaintUtilsTest, TestMultiByte) {
    // 312 = 0b100111000
    // Vaint '312': 0b0000 0111 1100 = 0x07, 0xC0
    uint8_t vaint_312[] = {0x07, 0xC0};
    test_conversions_of_value(312, vaint_312, 12, false);
}

TEST(VaintUtilsTest, TestVaintMax) {
    // Max Vaint in our implementation is 2^32 - 1
    // Vaint 2^32 - 1 = 0b0111 0111 0111 0111 0111 0111 0111 0111 0111 0111 1011
    uint8_t vaint_max[MAX_VAINT_LEN] = {0x77, 0x77, 0x77, 0x77, 0x77, 0xB0};
    test_conversions_of_value(UINT32_MAX, vaint_max, 44, false);
}

TEST(VaintUtilsTest, TestHalfByte) {
    // 43 = 0b101011
    // Vaint '43': 0b0011 1101. We pad with some bytes before and after to check whether our utilities work with
    //  half byte reads. So, 0b1010 0011 1101 1010 = 0xA3, 0xDA
    uint8_t vaint_43_offset[] = {0xA3, 0xDA};
    EXPECT_EQ(43, VaintUtil_DecodeVaint(vaint_43_offset, true, NULL));

    // Ensure our decode method leaves padding bits untouched.
    uint8_t result[sizeof(vaint_43_offset)] = {0xA0, 0x0A};
    uint8_t result_len_bits;
    VaintUtil_EncodeVaint(43, true, result, &result_len_bits);
    EXPECT_EQ(8, result_len_bits);
    EXPECT_EQ(0, memcmp(vaint_43_offset, result, sizeof(vaint_43_offset)));
}