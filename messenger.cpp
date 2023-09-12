#include <iostream>
#include <bitset>

#include "messenger.hpp"
#include "util.hpp"

// Valid range 1 - 31
#define MASK_FIRST_N(n) (( 1 << (n) ) -  1)

#define DEF_TO_STR(def) #def
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define ARR_LEN(arr) ((sizeof(arr)) / sizeof(arr[0]))

#define BITS_PER_BYTE 8

#define BITS_TO_RANGE(num_bits) (((1 << ((num_bits) - 1)) - 1) | (1 << ((num_bits) - 1)))


#define FLAG_BITS 0x5

#define MSGR_FLAG_BITS 3
#define MSGR_FLAG_MAX BITS_TO_RANGE(MSGR_FLAG_BITS)

#define MSGR_NAME_LEN_BITS 4
#define MSGR_NAME_LEN_MAX BITS_TO_RANGE(MSGR_NAME_LEN_BITS)

#define MSGR_MSG_LEN_BITS 5
#define MSGR_MSG_LEN_MAX BITS_TO_RANGE(MSGR_MSG_LEN_BITS)

#define MSGR_CRC4_BITS 4
#define MSGR_CRC4_MAX BITS_TO_RANGE(MSGR_CRC4_BITS)

// Should endianness be considered?
// Ans: only for multi-byte entities. I am not sure about bits of header.
// Is it guaranteed that bits will be placed from least to most significant?
// Ans: Not guaranteed, it is implementation defined how to place bits left-to-right or right-to-left


namespace messenger {

// namespace detail declaration
namespace detail {

class msg_hdr_t {
// Should namespace or class be used for very small funcs?
public:
    typedef uint8_t raw_type[2];
private:
    raw_type m_hdr; // Should it really be private?
public:

    msg_hdr_t(): m_hdr() { } // Will it fill m_hdr (array type) with zeroes?

    msg_hdr_t(uint8_t flag, uint8_t name_len, uint8_t msg_len) {
        this->set_flag(flag);
        this->set_name_len(name_len);
        this->set_msg_len(msg_len);
    }

    uint8_t *begin();

    uint8_t *end();

    // Set flag bits of header
    inline void set_flag(uint8_t flag);

    // Get flag bits of header
    inline uint8_t get_flag();

    // Set name_len bits of header
    inline void set_name_len(uint8_t name_len);

    // Get name_len bits of header
    inline uint8_t get_name_len();

    // Set msg_len bits of header
    inline void set_msg_len(uint8_t msg_len);

    // Get msg_len bits of header
    inline uint8_t get_msg_len();

    // Set crc4 bits of header
    inline void set_crc4(uint8_t crc4_val);

    // Get crc4 bits of header
    inline uint8_t get_crc4();

    // Calculate crc4 of packet masking crc4 bits with zeroes (might consider revision)
    inline uint8_t calculate_crc4();

};

} // namespace detail

std::vector<uint8_t> make_buff(const msg_t & msg) {
    if(msg.name.empty()) {
        throw std::length_error("Message name is empty");
    }
    
    if(msg.text.empty()) {
        throw std::length_error("Message text is empty");
    }
    
    if(msg.name.size() >= MSGR_NAME_LEN_MAX) {
        throw std::length_error("Message name length exceeds maximum length " DEF_TO_STR(MSGR_NAME_LEN_MAX) );
    }

    // Vector to store result of func
    std::vector<uint8_t> res;

    std::string::size_type msg_text_len = msg.text.size(); // Size of uncovered msg.text
    std::string::size_type msg_text_offset = 0; // Beginning of packet's text within msg.text
    std::string::size_type msg_text_offset_end = -1; // End of packet's text within msg.text
    std::vector<uint8_t>::size_type header_offset = 0;
    int packet_text_len = 0; // Length of text of cur packet. Either MAX or trailing of msg.text
    // Loop per packet
    while(msg_text_len > 0) {
        uint8_t crc4_res = 0;
        packet_text_len = MIN(msg_text_len, MSGR_MSG_LEN_MAX);

        // Header part of packet
        detail::msg_hdr_t header(FLAG_BITS, msg.name.size(), packet_text_len);
        crc4_res = header.calculate_crc4(); // Calculate crc4, masking crc4 bits with zeroes
        // Store cur loc of header within vector and make space for it. Header will be placed later
        header_offset = res.size();
        res.resize(res.size() + sizeof(header));

        // Sender (?) Name part of packet
        for(std::string::size_type i = 0; i < msg.name.size(); i++) {
            std::string::value_type val = msg.name[i];
            crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE); // Calculate crc4 per byte of name
            res.push_back( static_cast<uint8_t>(val) );
        }
        
        // Text part of packet
        for(msg_text_offset_end = msg_text_offset + packet_text_len ; 
            msg_text_offset < msg_text_offset_end;
            msg_text_offset++ ) {
            std::string::value_type val = msg.text[msg_text_offset];
            crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE); // Calculate crc4 per byte of text
            res.push_back( static_cast<uint8_t>(val) );
        }
        
        // Place header into packet with calculated crc4
        header.set_crc4(crc4_res);

        // Should endianness be really considered?
        if(util::is_little_endian()) {
            // Modify vector using pointers (not sure if there is other safer way to modify contiguous bytes)
            std::copy(header.begin(), header.end(), res.begin() + header_offset);
        } else {
            // It does not flip endianness, it just changes start of copy procedure
            // std::copy_backward(hdr_beg_ptr, hdr_end_ptr, res.begin() + header_offset);
            throw std::runtime_error("Big endian conversion is required");
        }

        // Reduce size of uncovered text
        msg_text_len -= packet_text_len;
    }

    return res;
}

// Function is not recoverable.
// Once discrepancy is found, previously parsed packets can not be retrieved
// Could consider providing valid packets 
msg_t parse_buff(std::vector<uint8_t> & buff) {
    std::string msg_name;
    std::string msg_text;
    bool is_name_retrieved = false;

    detail::msg_hdr_t header;
    
    if( buff.size() < 2 ) {
        throw std::runtime_error("Buffer does not contain bytes for header (at least 2)");
    }

    // Parse every packet
    std::vector<uint8_t>::iterator cur_iter = buff.begin();
    while(cur_iter != buff.end()) {
        // Copy to struct according to endianness. Byte with flag comes first
        if(util::is_little_endian()) {
            std::copy(cur_iter, cur_iter + sizeof(header), header.begin());
        } else {
            // It does not flip endiannes, it just changes start of copy procedure
            // std::copy_backward(cur_iter, cur_iter + sizeof(header), hdr_beg_ptr);
            throw std::runtime_error("Big endian conversion is required");
        }
        // Is it guaranteed that vector has always byte elements? Maybe increment in other way?
        cur_iter += sizeof(header);

        // Check if flag is valid
        if (header.get_flag() != FLAG_BITS)
        {
            throw std::runtime_error("Flag bit is not valid");
        }

        uint8_t crc4_packet = header.get_crc4(); // CRC4 retrieved from packet
        uint8_t crc4_real = header.calculate_crc4(); // CRC4 calculated from packet

        // Check if name & msg bits are valid.
        if(header.get_name_len() == 0) {
            throw std::runtime_error("NAME_LEN bits are empty");
        }

        if(header.get_msg_len() == 0) {
            throw std::runtime_error("MSG_LEN bits are empty");
        }

        // Retreive remaining bytes within packet
        std::vector<uint8_t>::size_type remaining_bytes = header.get_name_len(); // Casts to size_type
        remaining_bytes += header.get_msg_len();

        // Check if the remaining bytes of packet do not exceed actual size of buffer
        if(remaining_bytes > (buff.end() - cur_iter)) {
            throw std::runtime_error("Invalid name_len / msg_len fields. Indicated size exceeds actual size of buffer");
        }

        // Calculate crc4 of name and text parts of packet.
        for(std::vector<uint8_t>::iterator crc4_iter = cur_iter;
            crc4_iter != cur_iter + remaining_bytes; crc4_iter++) {
            crc4_real = util::crc4(crc4_real, *crc4_iter, BITS_PER_BYTE);
        }

        if(crc4_real != crc4_packet) {
            throw std::runtime_error("Invalid CRC4");
        }

        // Copy name
        std::string tmp_str(cur_iter, cur_iter + header.get_name_len()); // can be done with ostream_iterator

        // Check if name persists across packets
        if(!is_name_retrieved) {
            msg_name = std::move(tmp_str);
            is_name_retrieved = true;
        } else if(msg_name != tmp_str) {
            throw std::runtime_error("Sender names do not match accross packets");
        }

        cur_iter += header.get_name_len();
        // Copy message text
        std::vector<uint8_t>::iterator msg_txt_iter_end = cur_iter + header.get_msg_len();
        while(cur_iter != msg_txt_iter_end)
            msg_text += *cur_iter++; // Not sure if there is better way to do this
        
    }

    return msg_t(msg_name, msg_text);
}

/**
 * Covers details of operation
*/
namespace detail {

    uint8_t *msg_hdr_t::begin() {
        return reinterpret_cast<uint8_t *>(&this->m_hdr);
    }

    uint8_t *msg_hdr_t::end() {
        return reinterpret_cast<uint8_t *>(&this->m_hdr) + sizeof(this->m_hdr);
    }

    // Set flag bits of header
    inline void msg_hdr_t::set_flag(uint8_t flag) {
        this->m_hdr[0] &= ~(MASK_FIRST_N(MSGR_FLAG_BITS)); // Remove prev value of flag bits;
        this->m_hdr[0] |= flag & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Get flag bits of header
    inline uint8_t msg_hdr_t::get_flag() {
        return this->m_hdr[0] & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Set name_len bits of header
    inline void msg_hdr_t::set_name_len(uint8_t name_len) {
        this->m_hdr[0] &= ~( MASK_FIRST_N(MSGR_NAME_LEN_BITS) << MSGR_FLAG_BITS );  // Remove prev value of name_len bits
        this->m_hdr[0] |= (name_len & MASK_FIRST_N(MSGR_NAME_LEN_BITS)) << MSGR_FLAG_BITS;
    }

    // Get name_len bits of header
    inline uint8_t msg_hdr_t::get_name_len() {
        return (this->m_hdr[0] >> MSGR_FLAG_BITS) & MASK_FIRST_N(MSGR_NAME_LEN_BITS);
    }

    // Set msg_len bits of header
    inline void msg_hdr_t::set_msg_len(uint8_t msg_len) {
        this->m_hdr[0] &= ~(1 << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)); // Remove 1st bit of msg_len in 0th byte
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1 /* exclude 1 from tot */));
        this->m_hdr[0] |= (msg_len & 1) << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS);
        this->m_hdr[1] |= (msg_len >> 1) & (MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1));
    }

    // Get msg_len bits of header
    inline uint8_t msg_hdr_t::get_msg_len() {
        uint8_t res = (this->m_hdr[0] >> (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)) & 1;
        return res | ((this->m_hdr[1] & MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1)) << 1)  ;
    }

    // Set crc4 bits of header
    inline void msg_hdr_t::set_crc4(uint8_t crc4_val) {
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_CRC4_BITS) << (MSGR_MSG_LEN_BITS - 1));
        this->m_hdr[1] |= (crc4_val & MASK_FIRST_N(MSGR_CRC4_BITS)) << (MSGR_MSG_LEN_BITS - 1);
    }

    // Get crc4 bits of header
    inline uint8_t msg_hdr_t::get_crc4() {
        return (this->m_hdr[1] >> (MSGR_MSG_LEN_BITS - 1)) & MASK_FIRST_N(MSGR_CRC4_BITS);
    }

    // Calculate crc4 of packet masking crc4 bits with zeroes (might consider revision)
    inline uint8_t msg_hdr_t::calculate_crc4() {
        uint8_t res = 0;
        // Not safe in case of concurrent access
        uint8_t tmp_crc4 = this->get_crc4();
        this->set_crc4(0);
        uint8_t *beg = reinterpret_cast<uint8_t *>(this->m_hdr);
        uint8_t *end = beg + sizeof(this->m_hdr);
        while(beg < end) {
            res = util::crc4(res, *beg++, BITS_PER_BYTE);
        }

        this->set_crc4(tmp_crc4);

        return res;
    }

} // namespace detail

} // namespace messenger