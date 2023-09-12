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

// Push sequentially single packet from stream of text 
std::string::const_iterator push_single_packet (
    const std::string &name, 
    std::string::const_iterator msg_begin, 
    std::string::const_iterator msg_end, 
    std::vector<uint8_t> &out_vec
);

std::vector<uint8_t>::const_iterator parse_single_packet (
    std::vector<uint8_t>::const_iterator buf_begin,
    std::vector<uint8_t>::const_iterator buf_end,
    std::string &out_name,
    std::string &out_text
);

} // namespace detail


std::vector<uint8_t> make_buff(const msg_t & msg) {
    if(msg.name.empty()) throw std::length_error("messenger: sender's name is empty");
    
    if(msg.text.empty()) throw std::length_error("messenger: text is empty");
    
    if(msg.name.size() >= MSGR_NAME_LEN_MAX)
        throw std::length_error("messenger: name length exceeds maximum length " DEF_TO_STR(MSGR_NAME_LEN_MAX) );

    // Vector to store result of func
    std::vector<uint8_t> res;

    std::string::const_iterator next_packet_pos = msg.text.begin();

    // Loop per packet
    while(next_packet_pos != msg.text.cend())
        next_packet_pos = detail::push_single_packet(msg.name, next_packet_pos, msg.text.cend(), /* out */ res);

    return res;
}

// Function is not recoverable.
// Once discrepancy is found, previously parsed packets can not be retrieved
// Could consider providing valid packets 
msg_t parse_buff(std::vector<uint8_t> & buff) {
    std::string msg_name;
    std::string msg_text;
    bool is_name_retrieved = false;
    
    if( buff.size() < 2 )
        throw std::runtime_error("Buffer does not contain bytes for header (at least 2)");

    // Parse every packet
    std::vector<uint8_t>::const_iterator cur_iter = buff.begin();
    while(cur_iter != buff.end()) {
        std::string tmp_name;
        std::string tmp_text;

        cur_iter = detail::parse_single_packet(cur_iter, buff.cend(), tmp_name, tmp_text);

        // Check if name persists across packets
        if(!is_name_retrieved) {
            msg_name = std::move(tmp_name);
            is_name_retrieved = true;
        } else if(msg_name != tmp_name) {
            throw std::runtime_error("Sender names do not match accross packets");
        }

        // Add retrieved text
        msg_text += tmp_text;
    }

    return msg_t(msg_name, msg_text);
}

/**
 * Covers details of operation
*/
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
    
    uint8_t *begin() {
        return reinterpret_cast<uint8_t *>(&this->m_hdr);
    }

    uint8_t *end() {
        return reinterpret_cast<uint8_t *>(&this->m_hdr) + sizeof(this->m_hdr);
    }

    // Set flag bits of header
    inline void set_flag(uint8_t flag) {
        this->m_hdr[0] &= ~(MASK_FIRST_N(MSGR_FLAG_BITS)); // Remove prev value of flag bits;
        this->m_hdr[0] |= flag & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Get flag bits of header
    inline uint8_t get_flag() {
        return this->m_hdr[0] & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Set name_len bits of header
    inline void set_name_len(uint8_t name_len) {
        this->m_hdr[0] &= ~( MASK_FIRST_N(MSGR_NAME_LEN_BITS) << MSGR_FLAG_BITS );  // Remove prev value of name_len bits
        this->m_hdr[0] |= (name_len & MASK_FIRST_N(MSGR_NAME_LEN_BITS)) << MSGR_FLAG_BITS;
    }

    // Get name_len bits of header
    inline uint8_t get_name_len() {
        return (this->m_hdr[0] >> MSGR_FLAG_BITS) & MASK_FIRST_N(MSGR_NAME_LEN_BITS);
    }

    // Set msg_len bits of header
    inline void set_msg_len(uint8_t msg_len) {
        this->m_hdr[0] &= ~(1 << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)); // Remove 1st bit of msg_len in 0th byte
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1 /* exclude 1 from tot */));
        this->m_hdr[0] |= (msg_len & 1) << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS);
        this->m_hdr[1] |= (msg_len >> 1) & (MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1));
    }

    // Get msg_len bits of header
    inline uint8_t get_msg_len() {
        uint8_t res = (this->m_hdr[0] >> (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)) & 1;
        return res | ((this->m_hdr[1] & MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1)) << 1)  ;
    }

    // Set crc4 bits of header
    inline void set_crc4(uint8_t crc4_val) {
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_CRC4_BITS) << (MSGR_MSG_LEN_BITS - 1));
        this->m_hdr[1] |= (crc4_val & MASK_FIRST_N(MSGR_CRC4_BITS)) << (MSGR_MSG_LEN_BITS - 1);
    }

    // Get crc4 bits of header
    inline uint8_t get_crc4() {
        return (this->m_hdr[1] >> (MSGR_MSG_LEN_BITS - 1)) & MASK_FIRST_N(MSGR_CRC4_BITS);
    }

    // Calculate crc4 of packet masking crc4 bits with zeroes (might consider revision)
    inline uint8_t calculate_crc4() {
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

};

std::string::const_iterator push_single_packet (
    const std::string &name, 
    std::string::const_iterator msg_begin, 
    std::string::const_iterator msg_end, 
    std::vector<uint8_t> &out_vec
) {
    // Length of message of packet
    std::string::size_type packet_msg_len = MIN(msg_end - msg_begin, MSGR_MSG_LEN_MAX);
    
    // Header part of packet
    msg_hdr_t header(FLAG_BITS, name.size(), packet_msg_len);
    
    uint8_t crc4_res = 0;
    crc4_res = header.calculate_crc4(); // Calculate crc4, masking crc4 bits with zeroes
    // Store cur loc of header within vector and make space for it. Header will be placed later
    std::vector<uint8_t>::size_type header_offset = out_vec.size();
    out_vec.resize(header_offset + sizeof(header));

    // Calculate crc4 and push name part of packet
    for(std::string::size_type i = 0; i < name.size(); i++) {
        std::string::value_type val = name[i];
        crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE); // Calculate crc4 per byte of name
        out_vec.push_back( static_cast<uint8_t>(val) );
    }
    
    // Text part of packet
    for(std::string::const_iterator iter = msg_begin, end = msg_begin + packet_msg_len; 
        iter != end;
        iter++ ) {
        std::string::value_type val = *iter;
        crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE); // Calculate crc4 per byte of text
        out_vec.push_back( static_cast<uint8_t>(val) );
    }
    
    // Place header into packet with calculated crc4
    header.set_crc4(crc4_res);

    // Should endianness be really considered?
    if(util::is_little_endian()) {
        // Modify vector using pointers (not sure if there is other safer way to modify contiguous bytes)
        std::copy(header.begin(), header.end(), out_vec.begin() + header_offset);
    } else {
        // It does not flip endianness, it just changes start of copy procedure
        // std::copy_backward(hdr_beg_ptr, hdr_end_ptr, res.begin() + header_offset);
        throw std::runtime_error("messenger: big endian conversion is required");
    }

    return msg_begin + packet_msg_len;
}

std::vector<uint8_t>::const_iterator parse_single_packet (
    std::vector<uint8_t>::const_iterator buf_begin,
    std::vector<uint8_t>::const_iterator buf_end,
    std::string &out_name,
    std::string &out_text
) {
    msg_hdr_t header;
    std::vector<uint8_t>::const_iterator cur_iter = buf_begin;

    if(util::is_little_endian()) {
        std::copy(cur_iter, cur_iter + sizeof(header), header.begin());
    } else {
        // It does not flip endiannes, it just changes start of copy procedure
        // std::copy_backward(cur_iter, cur_iter + sizeof(header), hdr_beg_ptr);
        throw std::runtime_error("messenger: big endian conversion is required");
    }
    // Is it guaranteed that vector has always byte elements? Maybe increment in other way?
    cur_iter += sizeof(header);

    // Check if flag is valid
    if (header.get_flag() != FLAG_BITS) throw std::runtime_error("messenger: flag bit is invalid");

    uint8_t crc4_packet = header.get_crc4(); // CRC4 retrieved from packet
    uint8_t crc4_real = header.calculate_crc4(); // CRC4 calculated from packet

    // Check if name & msg bits are valid.
    if(header.get_name_len() == 0) throw std::runtime_error("messenger: name_len bits are empty");

    if(header.get_msg_len() == 0) throw std::runtime_error("messenger: msg_len bits are empty");

    // Retreive remaining bytes within packet
    std::vector<uint8_t>::size_type remaining_bytes = header.get_name_len(); // Casts to size_type
    remaining_bytes += header.get_msg_len();

    // Check if the remaining bytes of packet do not exceed actual size of buffer
    if(remaining_bytes > (buf_end - cur_iter))
        throw std::runtime_error("messenger: invalid name_len / msg_len fields. Indicated size exceeds actual size of buffer");

    // Calculate crc4 of name and text parts of packet.
    for(std::vector<uint8_t>::const_iterator crc4_iter = cur_iter;
        crc4_iter != cur_iter + remaining_bytes; crc4_iter++) {
        crc4_real = util::crc4(crc4_real, *crc4_iter, BITS_PER_BYTE);
    }

    if(crc4_real != crc4_packet) {
        throw std::runtime_error("Invalid CRC4");
    }

    // Copy name
    std::string tmp_str(cur_iter, cur_iter + header.get_name_len()); // can be done with ostream_iterator
    out_name = tmp_str;
    cur_iter += header.get_name_len();

    // Copy message text
    std::vector<uint8_t>::const_iterator msg_txt_iter_end = cur_iter + header.get_msg_len();
    while(cur_iter != msg_txt_iter_end)
        out_text += *cur_iter++; // Not sure if there is better way to do this

    return cur_iter;
}

} // namespace detail

} // namespace messenger