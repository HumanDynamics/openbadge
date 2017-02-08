//
// Created by Andrew Bartow on 1/19/17.
//

#ifndef OPENBADGE_VAINT_UTILS_H
#define OPENBADGE_VAINT_UTILS_H

#include <stdint.h>
#include <stdbool.h>

/*
 * This class includes utilities useful for manipulating variable length integers, called 'vaints'. Uses a half-byte
 *   variable length integer encoding scheme.
 *
 *  Our data streams often include small values (<= 7) but rarely include large values (>7) and so we can save large
 *  amounts of space by using variable length by using variable length integers.
 *
 *  Our vaint implementation can store an integer with value X in 4*(log_2(X) / 3) bits. By using this encoding on voice
 *    data, we can save ~40% of our storage space.
 *
 *  Our vaints are unsigned and the maximum value supported for encoding and decoding is 2^32 - 1.
 */

/**
 * Encodes an integer value as a variable length integer into 'result' taking 'result_len_bits' bits.
 * @param value The value to encode into a variable length integer.
 * @param start_at_half_byte If true, indicates to start encoding at four bits past p_result start, l
 *         leaving existing value unmodified.
 * @param p_result The destination for the encoded variable length integer
 * @param p_result_len_bits A pointer to where to store the length of the resulting variable length integer, in bits.
 */
void VaintUtil_EncodeVaint(const uint32_t value, const bool start_at_half_byte, uint8_t * p_result, uint8_t * p_result_len_bits);

/**
 * Decodes a variable length integer into an unsigned integer.
 * @param vaint The location of the start of the variable length integer.
 * @param offset If true, indicates to start decoding at four bits past vaint start.
 * @param p_len_bits. Optional. Pointer to integer to hold length of decoded vaint. Will be ignored if null.
 * @return The decoded value.
 */
uint32_t VaintUtil_DecodeVaint(const uint8_t * vaint, const bool start_at_half_byte, uint8_t * p_len_bits);

#endif //OPENBADGE_VAINT_UTILS_H
