#include <iostream>
#include <cassert>

#include "messenger.hpp"
#include "msg_hdr.hpp"
#include "util.hpp"

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

} // namespace detail


std::vector<uint8_t> make_buff(const msg_t & msg) {
    // Interface logic: Text and name cant be empty
    if(msg.name.empty()) throw std::length_error("messenger: name is empty");

    if(msg.name.size() > MSGR_NAME_LEN_MAX) throw std::length_error("messenger: name is too long");

    if(msg.text.empty()) throw std::length_error("messenger: text is empty");

    // Vector to store result of func
    std::vector<uint8_t> res;

    // As packet has limit on text size, divide text to several packets
    std::string::const_iterator next_text_pos = msg.text.begin();
    while(next_text_pos != msg.text.cend())
        next_text_pos = detail::push_single_packet(msg.name, next_text_pos, msg.text.cend(), /* out */ res);

    return res;
}

// Function is not recoverable.
// Once discrepancy is found, previously parsed packets can not be retrieved
msg_t parse_buff(std::vector<uint8_t> & buff) {
    std::string msg_name;
    std::string msg_text;
    bool is_name_retrieved = false;

    // Parse every packet
    std::vector<uint8_t>::const_iterator cur_iter = buff.begin();
    while(cur_iter != buff.end()) {
        std::string tmp_name;
        std::string tmp_text;

        // Throws runtime_error on: Invalid CRC4, invalid buff length to construct packet
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

class msg_packet_t {

private:
    uint8_t m_raw[sizeof(msg_hdr_view_t::hdr_raw_t) + MSGR_NAME_LEN_MAX + MSGR_MSG_LEN_MAX];
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
    ) {
        // Check if name is valid
        assertm(!name.empty() && name.size() <= MSGR_NAME_LEN_MAX, "msg_packet_t: name has wrong size");
        // Check validity of msg
        assertm(msg_beg < msg_end, "msg_packet_t: packet message has wrong iterators");

        std::string::size_type packet_msg_len = std::min(msg_end - msg_beg, 
            static_cast<std::string::iterator::difference_type>(MSGR_MSG_LEN_MAX) );

        uint8_t *m_raw_iter = &m_raw[sizeof(msg_hdr_view_t::hdr_raw_t)];
        
        // Copy name into packet
        std::copy(name.cbegin(), name.cend(), m_raw_iter);
        m_raw_iter += name.size();
        // Copy msg into packet
        std::copy(msg_beg, msg_beg + packet_msg_len, m_raw_iter);


        // Calculate crc4. Have to set vals of header first, before calculating crc4
        msg_hdr_mod_t hdr_modifier(&m_raw[0], name.size(), packet_msg_len, 0);
        uint8_t crc4_res = util::crc4_packet(begin(), end());
        // Place calculated crc4 in header
        hdr_modifier.set_crc4(crc4_res);
    }

    msg_packet_t(
        std::vector<uint8_t>::const_iterator buf_beg,
        std::vector<uint8_t>::const_iterator buf_end
    ) {
        // Check if buffer has at least enough bytes for header
        assertm((buf_end - buf_beg) >= sizeof(msg_hdr_view_t::hdr_raw_t), "msg_packet_t: buffer does not contain enough bytes");
        static_assert(util::endian::native == util::endian::little, "messenger: big endian conversion is not supported");

        msg_hdr_view_t buf_hdr_view(buf_beg.base());
        // Interface logic: If flag of incoming buffer is wrong, send runtime_error
        if(buf_hdr_view.get_flag() != FLAG_BITS)
            throw std::runtime_error("messenger: msg_packet_t: invalid flag bits") ;

        size_t packet_size = sizeof(msg_hdr_view_t::hdr_raw_t) + buf_hdr_view.get_name_len() + buf_hdr_view.get_msg_len();
        if(buf_beg + packet_size > buf_end)
            throw std::runtime_error("messenger: msg_packet_t: indicated name & msg size exceeds packet size");

        // Validate crc4
        uint8_t crc4_packet = buf_hdr_view.get_crc4(); // CRC4 retrieved from packet
        uint8_t crc4_real = util::crc4_packet(buf_beg.base(), (buf_beg + packet_size).base());

        if(crc4_real != crc4_packet)
            throw std::runtime_error("messenger: msg_packet_t: invalid CRC4");
        
        assertm(packet_size <= ARR_LEN(m_raw), "msg_packet_t: packet size is falsely larger");
        std::copy(buf_beg, buf_beg + packet_size, m_raw);

    }

    std::string name() {
        msg_hdr_view_t hdr_view(&m_raw[0]);
        const uint8_t *name_beg = begin() + sizeof(msg_hdr_view_t::hdr_raw_t);

        return std::string(name_beg, name_beg + hdr_view.get_name_len());
    }

    std::string msg() {
        msg_hdr_view_t hdr_view(&m_raw[0]);
        const uint8_t *msg_beg = begin() + sizeof(msg_hdr_view_t::hdr_raw_t) + hdr_view.get_name_len();

        return std::string(msg_beg, msg_beg + hdr_view.get_msg_len());
    }

    // Beginning of packet
    const uint8_t *begin() {
        return &m_raw[0];
    }

    // End of packet
    const uint8_t *end() {
        msg_hdr_view_t hdr_view(&m_raw[0]);
        return begin() + (sizeof(msg_hdr_view_t::hdr_raw_t) 
               + hdr_view.get_name_len()  + hdr_view.get_msg_len() );
    }

    size_t size() {
        return end() - begin();
    }

private:
    // /**
    //  * Helper func to copy name & msg from name string and msg range
    //  * 
    //  * @param name sender's name
    //  * @param msg_beg start of message
    //  * @param msg_end end of message
    //  * 
    //  * @note suffix of message maybe ignored due to max size of packet
    // */
    // void copy_into_name_and_msg(
    //     const std::string &name,
    //     std::string::const_iterator msg_beg,
    //     std::string::const_iterator msg_end
    // ) {
    //     uint8_t *buf_iter = &m_raw[sizeof(msg_hdr_t)];
    //     uint8_t *buf_end = &m_raw[0] + ARR_LEN(m_raw);

    //     // Check if wrong name/msg are passed
    //     // TODO: Change to assert
    //     if(m_hdr_ptr->get_name_len() != name.size()) 
    //         throw std::runtime_error("messenger: msg_packet_t: actual name size is not equal to indicated name size");

    //     // TODO: Change to assert
    //     size_t size_to_copy = m_hdr_ptr->get_msg_len();
    //     if(size_to_copy > (msg_end - msg_beg) )
    //         throw std::runtime_error("messenger: msg_packet_t: actual message size is less than indicated message size");

    //     // Copy name
    //     buf_iter = util::copy_string_to_buf(name.cbegin(), name.cend(), buf_iter, buf_end);
    //     // Copy msg
    //     buf_iter = util::copy_string_to_buf(msg_beg, msg_beg + size_to_copy, buf_iter, buf_end);

    // }

    // /**
    //  * Helper func to copy name & msg from vector buffer
    //  * 
    //  * @param buf_beg beginning of buffer
    //  * @param buf_end end of buffer
    //  * 
    //  * @note suffix of buffer maybe ignored due to limited sizes of name and message, defined in header
    // */
    // void copy_into_name_and_msg(
    //     std::vector<uint8_t>::const_iterator buf_beg,
    //     std::vector<uint8_t>::const_iterator buf_end
    // ) {
    //     // Retreive remaining bytes within packet
    //     std::vector<uint8_t>::size_type remaining_bytes = m_hdr_ptr->get_name_len(); // Casts to size_type
    //     remaining_bytes += m_hdr_ptr->get_msg_len();

    //     // Check if the remaining bytes of packet do not exceed actual size of buffer
    //     if(remaining_bytes > (buf_end - buf_beg))
    //         throw std::runtime_error("messenger: msg_packet_t: invalid name_len / msg_len fields. Indicated size exceeds actual size of buffer");

    //     if(remaining_bytes > (ARR_LEN(m_raw) - sizeof(msg_hdr_t)) ) // Impossible to happen, due to max vals of flags
    //         throw std::runtime_error("messenger: msg_packet_t: indicated name & msg size exceeds packet size");

    //     // [buf_beg, buf_beg + remaining_bytes] range's size is within [buf_beg, buf_end] and m_raw
    //     std::copy(buf_beg, buf_beg + remaining_bytes, &m_raw[sizeof(msg_hdr_t)]);
    // }

};

std::string::const_iterator push_single_packet (
    const std::string &name, 
    std::string::const_iterator msg_begin, 
    std::string::const_iterator msg_end, 
    std::vector<uint8_t> &out_vec
) {
    msg_packet_t packet(name, msg_begin, msg_end);

    static_assert(util::endian::native == util::endian::little, "messenger: big endian conversion is not supported");
        
    std::back_insert_iterator<std::vector<uint8_t>> vec_iter(out_vec);
    std::copy(packet.begin(), packet.end(), vec_iter);
    
    size_t msg_size_packed = packet.size()/*size of whole packet*/ - name.size() - sizeof(msg_hdr_view_t::hdr_raw_t);
    return msg_begin + msg_size_packed;
}

std::vector<uint8_t>::const_iterator parse_single_packet (
    std::vector<uint8_t>::const_iterator buf_beg,
    std::vector<uint8_t>::const_iterator buf_end,
    std::string &out_name,
    std::string &out_text
) {
    if( (buf_end - buf_beg) < sizeof(msg_hdr_view_t::hdr_raw_t))
        throw std::runtime_error("messenger: msg_packet_t: buffer does not contain enough bytes for header");

    msg_packet_t packet(buf_beg, buf_end);

    out_name = packet.name();
    out_text = packet.msg();

    return buf_beg + packet.size();
}

} // namespace detail

} // namespace messenger