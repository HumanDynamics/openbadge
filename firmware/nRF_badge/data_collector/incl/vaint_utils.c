//
// Created by Andrew Bartow on 1/19/17.
//

#include <string.h>
#include <app_error.h>

#include "vaint_utils.h"
#include "debug_log.h"

/**
 * Variable Integer Encoding Schema:
 *
 * We define a variable length integer encoding scheme to store unsigned integers of size [0, UINT32_MAX]
 *   that compactly stores very small values. (<= 7)
 *
 * We do this by using a variable length integer encoding scheme on the level of "nibbles" (4 bit integers).
 *
 * Each payload nibble contains three payload bits in the least significant bits [0:2] and a flag in the third bit
 *   that indicates if more bits remain in the integer. (If the bit is set, that nibble is the last nibble in
 *   the encoding of this integer).
 *
 * The nibbles in our variable length integer bytestream are read left to write, and the least significant payload bits
 *   are encoded first. All payload bits are concatenated to produce the encoded value.
 *
 * We refer to each nibble length combination of three payload bits and a continue flag as a "vaint cycle", as
 *   this pattern repeats over and over again until the end of the variable length integer.
 *
 *  Example: 617 is in binary '1001101001'.
 *     This is split into four groups of three payload bits each => 001 001 101 001
 *     Then it is written with flags with the least significant bits first => 0001 0101 0001 1001 => 0x13 0x19
 *
 *     To interpret this string, we can concatenate the payload bits:
 *         0[001], 0[101], 0[001], 1[001] => 001001101001 => 1001101001 => 617
 *
 */

#define BITS_PER_NIBBLE              4
#define BITS_PER_BYTE                (BITS_PER_NIBBLE * 2)
#define BITS_PER_UINT32              (sizeof(uint32_t) * BITS_PER_BYTE)
#define VAINT_CYCLE_LEN_BITS         BITS_PER_NIBBLE
#define PAYLOAD_BITS_PER_VAINT_CYCLE (VAINT_CYCLE_LEN_BITS - 1)

#define VAINT_FLAG_MASK              (1 << PAYLOAD_BITS_PER_VAINT_CYCLE)
#define CONTINUE_FLAG                0
#define FINISHED_FLAG                (1 << PAYLOAD_BITS_PER_VAINT_CYCLE)

// The NRF51 is little endian, so when we speak of bits being "first", we mean that they are least significant.

// Every vaint cycle starts at a nibble boundary, but we read our integers left to write and our chip is
//   little endian, so the first nibble of a vaint byte actually starts at the fourth bit of the byte,
//   and the second nibble of a vaint byte starts at the first bit of the byte.
// This can be really confusing to work with, so we abstract it away with this VAINT_NIBBLE_START() macro that takes
//   a position in a variable length integer where a cycle starts (from the start of the integer) and produces the
//   bit position of the start of the nibble that holds that cycle.
#define VAINT_NIBBLE_START(VAINT_POS)  ((VAINT_POS + BITS_PER_NIBBLE) % BITS_PER_BYTE)

// Returns the bits in value after the given position index (0-indexed), inclusive. If pos is larger than size of value,
//  returns zero.
static uint32_t bits_after_pos(uint32_t value, int pos) {
    if (pos >= BITS_PER_UINT32) return 0;
    return (UINT32_MAX << pos) & value;
}

// Returns the bits in value before the given position index (0-indexed), inclusive. If pos is larger than size of value,
//  returns value.
static uint32_t bits_before_pos(uint32_t value, int pos) {
    if ((pos + 1) >= BITS_PER_UINT32) return value;
    return ~(UINT32_MAX << (pos + 1)) & value;
}


// Returns the first 8 bits after start_pos in value, inclusive.
static uint8_t trim_to_byte(uint32_t value, int start_pos) {
    return (uint8_t) ((value >> start_pos) & ~(UINT32_MAX << 8));
}

// Returns the bits between start_pos and end_pos, inclusive, shifted to be at the start of the byte.
static uint8_t bits_in_range(uint32_t value, int start_pos, int end_pos) {
    uint32_t bits = bits_before_pos(value, end_pos) & bits_after_pos(value, start_pos);
    return trim_to_byte(bits, start_pos);
}

// Overwrites the bits in the indicated range of value with the first len bits of new_bits and returns the result.
static uint32_t overwrite_bits_in_range(uint32_t value, int start_pos, int len, uint32_t new_bits) {
    uint32_t overwrite_mask = (UINT32_MAX >>  (BITS_PER_UINT32 - len)) << start_pos;
    uint32_t value_with_range_erased = value & ~overwrite_mask;
    return value_with_range_erased | (new_bits << start_pos);
}

// Writes the payload_bits to the desired nibble of base_byte,
//  while preserving the other bits of base_byte, with the proper flag as dictated by encoding_last_cycle.
// NB: Only '0' and '4' are valid nibble start indexes.
static uint8_t construct_vaint_byte(uint8_t base_byte, int nibble_start_index, uint8_t payload_bits, bool encoding_last_cycle) {
    // Load base_byte into encoded_byte to preserve unaffected bits.
    uint8_t encoded_byte = base_byte;

    // Choose proper flag and apply it to our payload bits.
    uint8_t flag = encoding_last_cycle ? (uint8_t) FINISHED_FLAG : (uint8_t) CONTINUE_FLAG;
    uint8_t bits_with_flag = flag | payload_bits;

    return (uint8_t) overwrite_bits_in_range(encoded_byte, nibble_start_index, VAINT_CYCLE_LEN_BITS, bits_with_flag);
}

// Returns the next PAYLOAD_BITS_PER_VAINT_CYCLE bits after start_pos, inclusive. (0-indexed)
static uint8_t read_payload_bits_after_pos(uint32_t value, uint8_t start_pos) {
    return bits_in_range(value, start_pos, start_pos + PAYLOAD_BITS_PER_VAINT_CYCLE - 1);

}

void VaintUtil_EncodeVaint(const uint32_t value, const bool start_at_half_byte, uint8_t * p_result, uint8_t * p_result_len_bits) {
    uint8_t value_pos = 0;
    uint8_t result_pos = start_at_half_byte ? (uint8_t) VAINT_CYCLE_LEN_BITS : (uint8_t) 0;
    uint8_t result_len_bits = 0;

    // Encode all bits before last cycle into the vaint.
    while ((bits_after_pos(value, value_pos + PAYLOAD_BITS_PER_VAINT_CYCLE) != 0)) {
        p_result[result_pos / BITS_PER_BYTE] = construct_vaint_byte(p_result[result_pos / BITS_PER_BYTE],
                                                                    VAINT_NIBBLE_START(result_pos),
                                                                    read_payload_bits_after_pos(value, value_pos),
                                                                    false);

        value_pos += PAYLOAD_BITS_PER_VAINT_CYCLE;
        result_pos += VAINT_CYCLE_LEN_BITS;
        result_len_bits += VAINT_CYCLE_LEN_BITS;
    }

    // Encode the last payload bits into our vaint (notice encoding_last_cycle is TRUE).
    p_result[result_pos / BITS_PER_BYTE] = construct_vaint_byte(p_result[result_pos / BITS_PER_BYTE],
                                                                VAINT_NIBBLE_START(result_pos),
                                                                read_payload_bits_after_pos(value, value_pos),
                                                                true);
    result_len_bits += VAINT_CYCLE_LEN_BITS;

    *p_result_len_bits = result_len_bits;
}

// Returns the first VAINT_CYCLE_LEN_BITS from vaint after start_pos (inclusive).
static uint8_t vaint_cycle_after_pos(const uint8_t * vaint, uint8_t start_pos) {
    return bits_in_range(vaint[start_pos / BITS_PER_BYTE],
                         VAINT_NIBBLE_START(start_pos),
                         VAINT_NIBBLE_START(start_pos) + VAINT_CYCLE_LEN_BITS);
}

// Returns just the payload bits from a vaint_cycle.
static uint8_t payload_from_vaint_cycle(uint8_t vaint_cycle) {
    return bits_in_range(vaint_cycle, 0, PAYLOAD_BITS_PER_VAINT_CYCLE - 1);
}

uint32_t VaintUtil_DecodeVaint(const uint8_t * vaint, const bool start_at_half_byte, uint8_t * p_len_bits) {
    uint32_t value = 0;
    uint8_t value_pos = 0;
    uint8_t vaint_pos = start_at_half_byte ? (uint8_t) VAINT_CYCLE_LEN_BITS : (uint8_t) 0;
    uint8_t decoded_len_bits = 0;

    while ((vaint_cycle_after_pos(vaint, vaint_pos) & VAINT_FLAG_MASK) == CONTINUE_FLAG) {
        uint8_t vaint_cycle = vaint_cycle_after_pos(vaint, vaint_pos);
        uint8_t payload = payload_from_vaint_cycle(vaint_cycle);
        value = (payload << value_pos) + value;

        vaint_pos += VAINT_CYCLE_LEN_BITS;
        value_pos += PAYLOAD_BITS_PER_VAINT_CYCLE;
        decoded_len_bits += VAINT_CYCLE_LEN_BITS;
    }

    uint8_t vaint_cycle = vaint_cycle_after_pos(vaint, vaint_pos);
    uint8_t payload = payload_from_vaint_cycle(vaint_cycle);
    value = (payload << value_pos) + value;
    decoded_len_bits += VAINT_CYCLE_LEN_BITS;

    if (p_len_bits != NULL) *p_len_bits = decoded_len_bits;

    return value;
}