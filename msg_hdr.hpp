#ifndef MSG_HDR_H
#define MSG_HDR_H

#include <vector>
#include <cstdint>

#include "util.hpp"

#define FLAG_BITS 0x5

#define MSGR_FLAG_BITS 3
#define MSGR_FLAG_MAX BITS_TO_RANGE(MSGR_FLAG_BITS)

#define MSGR_NAME_LEN_BITS 4
#define MSGR_NAME_LEN_MAX BITS_TO_RANGE(MSGR_NAME_LEN_BITS)

#define MSGR_MSG_LEN_BITS 5
#define MSGR_MSG_LEN_MAX BITS_TO_RANGE(MSGR_MSG_LEN_BITS)

#define MSGR_CRC4_BITS 4
#define MSGR_CRC4_MAX BITS_TO_RANGE(MSGR_CRC4_BITS)

namespace messenger::detail {

class msg_hdr_view_t {
// Should namespace or class be used for very small funcs?
public:
    typedef uint8_t hdr_raw_t[2];
protected:
    hdr_raw_t * const m_hdr;
public:

    msg_hdr_view_t(const uint8_t *pos): m_hdr(
        // Dropping const, because msg_hdr_mod_t would have to either have own pointer, or drop const every time
        const_cast<hdr_raw_t *>(
            reinterpret_cast<const hdr_raw_t *>(pos)
        )
    ) { }

    inline uint8_t get_flag() {
        return (*m_hdr)[0] & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    inline uint8_t get_name_len() {
        return ((*m_hdr)[0] >> MSGR_FLAG_BITS) & MASK_FIRST_N(MSGR_NAME_LEN_BITS);
    }

    inline uint8_t get_msg_len() {
        uint8_t res = ((*m_hdr)[0] >> (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)) & 1;
        return res | (((*m_hdr)[1] & MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1)) << 1)  ;
    }

    inline uint8_t get_crc4() {
        return ((*m_hdr)[1] >> (MSGR_MSG_LEN_BITS - 1)) & MASK_FIRST_N(MSGR_CRC4_BITS);
    }

    inline uint8_t calculate_crc4() {
        uint8_t res = 0;
        
        hdr_raw_t tmp_crc4;
        
        const uint8_t *tmp_beg = reinterpret_cast<const uint8_t *>(&tmp_crc4);
        const uint8_t *tmp_end = tmp_beg + sizeof(hdr_raw_t);
        
        // Make local copy, so as to nullify crc4
        std::copy( reinterpret_cast<uint8_t *>(m_hdr), 
                   reinterpret_cast<uint8_t *>(m_hdr + 1) /* 1 element of type hdr_raw_t */,
                   reinterpret_cast<uint8_t *>(&tmp_crc4));

        // nullify crc4 bits
        tmp_crc4[1] &= ~(MASK_FIRST_N(MSGR_CRC4_BITS) << (MSGR_MSG_LEN_BITS - 1));

        res = util::crc4_range(res, tmp_beg, tmp_end);

        return res;
    }

};

class msg_hdr_mod_t : public msg_hdr_view_t {

public:
    msg_hdr_mod_t() = delete;

    msg_hdr_mod_t(uint8_t *pos): msg_hdr_view_t(pos) { }

    msg_hdr_mod_t(
        uint8_t *pos, uint8_t name_len, 
        uint8_t msg_len, uint8_t crc4_val
    ): msg_hdr_view_t(pos) { 
        set_flag(FLAG_BITS);
        set_name_len(name_len);
        set_msg_len(msg_len);
        set_crc4(crc4_val);
    }

    inline void set_flag(uint8_t flag) {
        (*m_hdr)[0] &= ~(MASK_FIRST_N(MSGR_FLAG_BITS)); // Remove prev value of flag bits;
        (*m_hdr)[0] |= flag & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Set name_len bits of header
    inline void set_name_len(uint8_t name_len) {
        (*m_hdr)[0] &= ~( MASK_FIRST_N(MSGR_NAME_LEN_BITS) << MSGR_FLAG_BITS );  // Remove prev value of name_len bits
        (*m_hdr)[0] |= (name_len & MASK_FIRST_N(MSGR_NAME_LEN_BITS)) << MSGR_FLAG_BITS;
    }

    // Set msg_len bits of header
    inline void set_msg_len(uint8_t msg_len) {
        (*m_hdr)[0] &= ~(1 << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)); // Remove 1st bit of msg_len in 0th byte
        (*m_hdr)[1] &= ~(MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1 /* exclude 1 from tot */));
        (*m_hdr)[0] |= (msg_len & 1) << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS);
        (*m_hdr)[1] |= (msg_len >> 1) & (MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1));
    }

    // Set crc4 bits of header
    inline void set_crc4(uint8_t crc4_val) {
        (*m_hdr)[1] &= ~(MASK_FIRST_N(MSGR_CRC4_BITS) << (MSGR_MSG_LEN_BITS - 1));
        (*m_hdr)[1] |= (crc4_val & MASK_FIRST_N(MSGR_CRC4_BITS)) << (MSGR_MSG_LEN_BITS - 1);
    }

};

} // namespace messenger::detail


#endif