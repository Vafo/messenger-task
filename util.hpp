// SPDX-License-Identifier: GPL-2.0-only
/*
 * crc4.c - simple crc-4 calculations.
 */
#ifndef MESSENGER_UTIL_H
#define MESSENGER_UTIL_H

#include <iostream>

#include <cstdint>
#include <cstddef>
#include <cassert>


#define ARR_LEN(arr) ((sizeof(arr)) / sizeof(arr[0]))

#define BITS_PER_BYTE 8

#define BITS_TO_RANGE(num_bits) (((1 << ((num_bits) - 1)) - 1) | (1 << ((num_bits) - 1)))
#define MASK_FIRST_N(n) BITS_TO_RANGE(n)

// Assert with message. ref: https://en.cppreference.com/w/cpp/error/assert
#define assertm(exp, msg) assert(((void)(msg), (exp)))

namespace messenger::util {
    
/**
 * crc4 - calculate the 4-bit crc of a value.
 * @c:    starting crc4
 * @x:    value to checksum
 * @bits: number of bits in @x to checksum
 *
 * Returns the crc4 value of @x, using polynomial 0b10111.
 *
 * The @x value is treated as left-aligned, and bits above @bits are ignored
 * in the crc calculations.
 * 
 * Originally taken from https://codebrowser.dev/linux/linux/lib/crc4.c.html
 */
uint8_t crc4(uint8_t c, uint64_t x, size_t bits);

/**
 * crc4 - calculate the 4-bit crc of range of bytes.
 * @c:    starting crc4
 * @beg:  beginning of range
 * @end:  end of range
 *
 * Returns the crc4 value of range [@beg, @end), using crc4 function.
 */
uint8_t crc4_range(uint8_t c, const uint8_t *beg, const uint8_t *end);

// Calculate crc4 of packet view
uint8_t crc4_packet(const uint8_t *beg, const uint8_t *end);

/**
 * endian - get endianness of machine
 * 
 * @details STL variant is implemented in C++20
 *          This implementation is taken from http://howardhinnant.github.io/endian.html#Implementation
*/
enum class endian
{
    little = 1234,
    big    = 4321,
    native = 1234
};

/**
 * @brief scoped_hex - set ostream to hex.
 * @details Helper class used to set std::hex output of ostream within scope of scoped_hex object
 *          Once reaching end of scope, scoped_hex sets back std::dec output of ostream
*/
class scoped_hex {

private:
    std::ostream &target_stream;
public:
    scoped_hex(std::ostream &stream): target_stream(stream) {
        target_stream << std::hex;
    }

    ~scoped_hex() {
        target_stream << std::dec;
    }
    
};

} // namespace messenger::util

#endif