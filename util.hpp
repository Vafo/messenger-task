// SPDX-License-Identifier: GPL-2.0-only
/*
 * crc4.c - simple crc-4 calculations.
 */
#ifndef MESSENGER_UTIL_H
#define MESSENGER_UTIL_H

#include <cstdint>

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
     */
    uint8_t crc4(uint8_t c, uint64_t x, int bits);
    
    // Checks if machine is Little Endian
    inline bool isLittleEndian() {
        short int number = 0x1;
        char *numPtr = (char*)&number;
        return (numPtr[0] == 1);
    }
}

#endif