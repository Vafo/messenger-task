#include "util.hpp"

namespace messenger::util {
    static const uint8_t crc4_tab[] = {
        0x0, 0x7, 0xe, 0x9, 0xb, 0xc, 0x5, 0x2,
        0x1, 0x6, 0xf, 0x8, 0xa, 0xd, 0x4, 0x3,
    };
    
    uint8_t crc4(uint8_t c, uint64_t x, size_t bits)
    {
        int i;
        /* mask off anything above the top bit */
        x &= (1ull << bits) - 1;
        /* Align to 4-bits */
        bits = (bits + 3) & ~0x3;
        /* Calculate crc4 over four-bit nibbles, starting at the MSbit */
        for (i = bits - 4; i >= 0; i -= 4)
            c = crc4_tab[c ^ ((x >> i) & 0xf)];
        return c;
    }
}