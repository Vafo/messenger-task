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
    std::vector<uint8_t>::const_iterator buf_beg,
    std::vector<uint8_t>::const_iterator buf_end,
    std::string &out_name,
    std::string &out_text
);

// Copy from string to uint8_t buffer
uint8_t *copy_string_to_buf(
    std::string::const_iterator str_beg, 
    std::string::const_iterator str_end, 
    uint8_t * const buf_beg, 
    uint8_t * const buf_end
);

} // namespace detail


std::vector<uint8_t> make_buff(const msg_t & msg) {
    // Vital validation check, as msg_packet_t cant track it, since while loop wont execute
    if(msg.text.empty()) throw std::length_error("messenger: text is empty");

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
            throw std::runtime_error("messenger: sender names do not match accross packets");
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

    msg_hdr_t(uint8_t name_len, uint8_t msg_len) {
        // Check lengths
        this->check_valid_len(name_len, msg_len);

        this->set_flag(FLAG_BITS); // Design defined valid value
        this->set_name_len(name_len);
        this->set_msg_len(msg_len);

    }

    // Copy constructor with modification
    // Is used to avoid setters
    msg_hdr_t(const msg_hdr_t &orig_header, uint8_t crc4) {
        *this = orig_header;
        this->set_crc4(crc4);
    }

    msg_hdr_t(const uint8_t *beg, const uint8_t *end) {
        if( (end - beg) != sizeof(msg_hdr_t) ) 
            throw std::runtime_error("messenger: msg_hdr_t: constructor received inappropriate range");

        std::copy(beg, end, &m_hdr[0]);
        
        // Check validity
        // We can check only flag, as buffer has already packed name & msg len bits
        this->check_valid_flag();
    }
    
    msg_hdr_t(
        std::vector<uint8_t>::const_iterator beg,
        std::vector<uint8_t>::const_iterator end
    ) {
        if( (end - beg) != sizeof(msg_hdr_t) ) 
            throw std::runtime_error("messenger: msg_hdr_t: constructor received inappropriate range");

        std::copy(beg, end, &m_hdr[0]);

        // Check validity
        // We can check only flag, as buffer has already packed name & msg len bits
        this->check_valid_flag();
    }

    const uint8_t *begin() const{
        return reinterpret_cast<const uint8_t *>(&this->m_hdr);
    }

    const uint8_t *end() const{
        return reinterpret_cast<const uint8_t *>(&this->m_hdr) + sizeof(this->m_hdr);
    }

    // Get flag bits of header
    inline uint8_t get_flag() {
        return this->m_hdr[0] & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Get name_len bits of header
    inline uint8_t get_name_len() {
        return (this->m_hdr[0] >> MSGR_FLAG_BITS) & MASK_FIRST_N(MSGR_NAME_LEN_BITS);
    }

    // Get msg_len bits of header
    inline uint8_t get_msg_len() {
        uint8_t res = (this->m_hdr[0] >> (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)) & 1;
        return res | ((this->m_hdr[1] & MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1)) << 1)  ;
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

private:
    // Setters are still internally used, to avoid mixing in bitwise ops

    // Set flag bits of header
    inline void set_flag(uint8_t flag) {
        this->m_hdr[0] &= ~(MASK_FIRST_N(MSGR_FLAG_BITS)); // Remove prev value of flag bits;
        this->m_hdr[0] |= flag & MASK_FIRST_N(MSGR_FLAG_BITS);
    }

    // Set name_len bits of header
    inline void set_name_len(uint8_t name_len) {
        this->m_hdr[0] &= ~( MASK_FIRST_N(MSGR_NAME_LEN_BITS) << MSGR_FLAG_BITS );  // Remove prev value of name_len bits
        this->m_hdr[0] |= (name_len & MASK_FIRST_N(MSGR_NAME_LEN_BITS)) << MSGR_FLAG_BITS;
    }

    // Set msg_len bits of header
    inline void set_msg_len(uint8_t msg_len) {
        this->m_hdr[0] &= ~(1 << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS)); // Remove 1st bit of msg_len in 0th byte
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1 /* exclude 1 from tot */));
        this->m_hdr[0] |= (msg_len & 1) << (MSGR_FLAG_BITS + MSGR_NAME_LEN_BITS);
        this->m_hdr[1] |= (msg_len >> 1) & (MASK_FIRST_N(MSGR_MSG_LEN_BITS - 1));
    }

    // Set crc4 bits of header
    inline void set_crc4(uint8_t crc4_val) {
        this->m_hdr[1] &= ~(MASK_FIRST_N(MSGR_CRC4_BITS) << (MSGR_MSG_LEN_BITS - 1));
        this->m_hdr[1] |= (crc4_val & MASK_FIRST_N(MSGR_CRC4_BITS)) << (MSGR_MSG_LEN_BITS - 1);
    }

    // Check if bit fields within packet are valid
    inline void check_valid_len(uint8_t name_len, uint8_t msg_len) {
        // Check if name & msg size are valid.
        if(name_len == 0 || name_len > MSGR_NAME_LEN_MAX) 
            throw std::runtime_error("messenger: msg_hdr_t: invalid name_len");

        if(msg_len == 0 || msg_len > MSGR_MSG_LEN_MAX)
            throw std::runtime_error("messenger: msg_hdr_t: invalid msg_len");
    }

    inline void check_valid_flag() {
        // Check if flag is valid
            if (this->get_flag() != FLAG_BITS) throw std::runtime_error("messenger: flag bit is invalid");
    }

};


class msg_packet_t {

private:
    uint8_t m_raw[sizeof(msg_hdr_t) + MSGR_NAME_LEN_MAX + MSGR_MSG_LEN_MAX];
    msg_hdr_t * const m_hdr_ptr; // Just to simplify access to header

public:
    /**
     * Construct packet from name and message
     * 
     * @param name sender's name
     * @param msg_beg message's beginning
     * @param msg_end message's end
     * 
     * @note first MSGR_MSG_LEN_MAX from message range will be included in packet ignoring rest 
    */
    msg_packet_t(
        const std::string &name,
        std::string::const_iterator msg_beg,
        std::string::const_iterator msg_end
    ): m_hdr_ptr( reinterpret_cast<msg_hdr_t * const>(&m_raw[0]) ) {

        std::string::size_type packet_msg_len = std::min(msg_end - msg_beg, 
            static_cast<std::string::iterator::difference_type>(MSGR_MSG_LEN_MAX) );
        
        // Initialize header 
        *m_hdr_ptr = msg_hdr_t(name.size(), packet_msg_len);

        uint8_t *m_raw_iter = &m_raw[sizeof(msg_hdr_t)];
        // Copy name & msg
        this->copy_into_name_and_msg(name, msg_beg, msg_end);

        // Calculate crc4
        uint8_t crc4_res = this->calculate_crc4();

        // Place header into packet with calculated crc4
        *m_hdr_ptr = msg_hdr_t(*m_hdr_ptr, crc4_res);
    }

    msg_packet_t(
        std::vector<uint8_t>::const_iterator buf_beg,
        std::vector<uint8_t>::const_iterator buf_end
    ): m_hdr_ptr( reinterpret_cast<msg_hdr_t * const>(&m_raw[0]) ) {

        std::vector<uint8_t>::const_iterator buf_iter = buf_beg;
        
        // Copy header part
        if( (buf_end - buf_beg) < sizeof(msg_hdr_t))
            throw std::runtime_error("messenger: msg_packet_t: buffer does not contain enough bytes for header");

        // Big endian conversion is not supported yet
        static_assert(util::endian::native != util::endian::big, "messenger: big endian conversion is not supported");

        if(util::endian::native == util::endian::little) {
            *m_hdr_ptr = msg_hdr_t(buf_iter, buf_iter + sizeof(msg_hdr_t));
        }
        // Is it guaranteed that vector has always byte elements? Maybe increment in other way?
        buf_iter += sizeof(msg_hdr_t);

        // Copy name and msg from buffer
        this->copy_into_name_and_msg(buf_iter, buf_end);

        // Validate crc4
        uint8_t crc4_packet = m_hdr_ptr->get_crc4(); // CRC4 retrieved from packet
        uint8_t crc4_real = this->calculate_crc4();

        if(crc4_real != crc4_packet) {
            throw std::runtime_error("messenger: msg_packet_t: invalid CRC4");
        }
    }

    // Are getters for name & msg len redundant?

    inline uint8_t get_name_len() {
        return m_hdr_ptr->get_name_len();
    }

    inline uint8_t get_msg_len() {
        return m_hdr_ptr->get_msg_len();
    }

    // Beginning of name in packet
    inline const uint8_t *name_begin() {
        return &m_raw[sizeof(msg_hdr_t)];
    }

    // End of name in packet
    inline const uint8_t *name_end() {
        return name_begin() + m_hdr_ptr->get_name_len();
    }

    // Beginning of msg in packet
    inline const uint8_t *msg_begin() {
        return name_end();
    }

    // End of name in packet
    inline const uint8_t *msg_end() {
        return msg_begin() + m_hdr_ptr->get_msg_len();
    }

    // Beginning of packet
    inline const uint8_t *begin() {
        return &m_raw[0];
    }

    // End of packet
    inline const uint8_t *end() {
        return &m_raw[0] + (sizeof(msg_hdr_t) + m_hdr_ptr->get_name_len() + m_hdr_ptr->get_msg_len() );
    }

private:
    /**
     * Helper func to copy name & msg from name string and msg range
     * 
     * @param name sender's name
     * @param msg_beg start of message
     * @param msg_end end of message
     * 
     * @note suffix of message maybe ignored due to max size of packet
    */
    void copy_into_name_and_msg(
        const std::string &name,
        std::string::const_iterator msg_beg,
        std::string::const_iterator msg_end
    ) {
        uint8_t *buf_iter = &m_raw[sizeof(msg_hdr_t)];
        uint8_t *buf_end = &m_raw[0] + ARR_LEN(m_raw);

        // Check if wrong name/msg are passed
        if(m_hdr_ptr->get_name_len() != name.size()) 
            throw std::runtime_error("messenger: msg_packet_t: actual name size is not equal to indicated name size");

        size_t size_to_copy = m_hdr_ptr->get_msg_len();
        if(size_to_copy > (msg_end - msg_beg) )
            throw std::runtime_error("messenger: msg_packet_t: actual message size is less than indicated message size");

        // Copy name
        buf_iter = copy_string_to_buf(name.cbegin(), name.cend(), buf_iter, buf_end);
        // Copy msg
        buf_iter = copy_string_to_buf(msg_beg, msg_beg + size_to_copy, buf_iter, buf_end);

    }

    /**
     * Helper func to copy name & msg from vector buffer
     * 
     * @param buf_beg beginning of buffer
     * @param buf_end end of buffer
     * 
     * @note suffix of buffer maybe ignored due to limited sizes of name and message, defined in header
    */
    void copy_into_name_and_msg(
        std::vector<uint8_t>::const_iterator buf_beg,
        std::vector<uint8_t>::const_iterator buf_end
    ) {
        // Retreive remaining bytes within packet
        std::vector<uint8_t>::size_type remaining_bytes = m_hdr_ptr->get_name_len(); // Casts to size_type
        remaining_bytes += m_hdr_ptr->get_msg_len();

        // Check if the remaining bytes of packet do not exceed actual size of buffer
        if(remaining_bytes > (buf_end - buf_beg))
            throw std::runtime_error("messenger: msg_packet_t: invalid name_len / msg_len fields. Indicated size exceeds actual size of buffer");

        if(remaining_bytes > (ARR_LEN(m_raw) - sizeof(msg_hdr_t)) ) // Impossible to happen, due to max vals of flags
            throw std::runtime_error("messenger: msg_packet_t: indicated name & msg size exceeds packet size");

        // [buf_beg, buf_beg + remaining_bytes] range's size is within [buf_beg, buf_end] and m_raw
        std::copy(buf_beg, buf_beg + remaining_bytes, &m_raw[sizeof(msg_hdr_t)]);
    }

    // Calculate crc4 of packet
    uint8_t calculate_crc4() {
        // Starting crc4
        uint8_t crc4_res = m_hdr_ptr->calculate_crc4();

        const uint8_t *crc4_iter = &m_raw[sizeof(msg_hdr_t)];
        const uint8_t *crc4_iter_end = crc4_iter + (m_hdr_ptr->get_name_len() + m_hdr_ptr->get_msg_len());

        // Calculate crc4 of name and text parts of packet.
        for( ; crc4_iter != crc4_iter_end; crc4_iter++) {
            crc4_res = util::crc4(crc4_res, *crc4_iter, BITS_PER_BYTE);
        }
        
        return crc4_res;
    }

};

// Copy from string to buffer of uint8_t elements
// checks if string can fit buffer
uint8_t *copy_string_to_buf(
    std::string::const_iterator str_beg, 
    std::string::const_iterator str_end, 
    uint8_t * const buf_beg, 
    uint8_t * const buf_end
) {
    if(buf_beg == NULL || buf_end == NULL)
        throw std::runtime_error("messenger: calc_crc4_and_push: buf pointer(s) is(are) NULL");

    // Check if buffer can hold given string
    if( (buf_end - buf_beg) < (str_end - str_beg) )
        throw std::runtime_error("messenger: calc_crc4_and_push: string does not fit into buffer");

    std::copy(str_beg, str_end, buf_beg);

    // Return shifted iterator within buffer
    return buf_beg + (str_end - str_beg);
}

std::string::const_iterator push_single_packet (
    const std::string &name, 
    std::string::const_iterator msg_begin, 
    std::string::const_iterator msg_end, 
    std::vector<uint8_t> &out_vec
) {
    
    // It is responsibility of caller, to check if message of appropriate size for 1 packet
    // As packet class is responsible for creating single packet
    // Yet multiple packets are needed for the caller
    // CHANGE: But it makes code easier if larger range is given to packet
    // It is up to packet to take enough data, and just leave the rest
    // And the caller can just get size of data taken, and move pointer
    
    // Create packet
    msg_packet_t packet(name, msg_begin, msg_end);

    // The code below is left just for sake of practice of using static_assert
    // Yet arguably, does header-packet has endianness? Is it considered multi-byte entity?
    // Or should header-packet's structure remain the same, regardless of endianness?
    // And if header of packets share endianness, should it be always big-endian?

    // Should endianness be really considered?
    static_assert(util::endian::native != util::endian::big, "messenger: big endian conversion is not supported");

    // Does compiler simplify this if statement, which has const condition?
    if(util::endian::native == util::endian::little) {
        
        // Is there better way to copy/append from array of elements to vector (?)
        for(const uint8_t *iter = packet.begin(), *iter_end = packet.end();
            iter != iter_end;
            iter++
        ) {
            out_vec.push_back(*iter);
        }
        
    }

    return msg_begin + packet.get_msg_len();
}

std::vector<uint8_t>::const_iterator parse_single_packet (
    std::vector<uint8_t>::const_iterator buf_beg,
    std::vector<uint8_t>::const_iterator buf_end,
    std::string &out_name,
    std::string &out_text
) {
    msg_packet_t parsed_packet(buf_beg, buf_end);

    out_name = std::string(parsed_packet.name_begin(), parsed_packet.name_end());
    out_text = std::string(parsed_packet.msg_begin(), parsed_packet.msg_end());

    return buf_beg + (parsed_packet.end() - parsed_packet.begin());
}

} // namespace detail

} // namespace messenger