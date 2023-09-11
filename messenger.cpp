#include <iostream>
#include <bitset>

#include "messenger.hpp"
#include "util.hpp"

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

// Valid range 1 - 31

// Should endianness be considered?
// Is it guaranteed that bits will be placed from least to most significant?
// Is the size of msg_hdr_t obj guaranteed to be 16 bits regardless of machine/compiler ?

// union msg_hdr_t {
//     uint8_t raw[2];
//     struct {
//         uint8_t flag : 3;
//         uint8_t name : 4;
//         uint8_t msg_len : 5;
//         uint8_t crc4 : 4;
//     } bits;
// };

namespace messenger
{
    struct msg_hdr_t {
        
        private:
        struct {
            uint8_t flag : 3;
            uint8_t name_len : 4;
            uint8_t msg_len_lsb : 1;
        } first;

        struct {
            uint8_t msg_len_rest : 4;
            uint8_t crc4 : 4;
        } second;


        public:

        msg_hdr_t() {}

        msg_hdr_t(uint8_t fg, uint8_t nl, uint8_t ml, uint8_t c4 = 0) {
            this->set_flag(fg);
            this->set_name_len(nl);
            this->set_msg_len(ml);
            this->set_crc4(c4);
        }

        // TODO Learn about inline and qualify small funcs
        inline void set_flag(uint8_t flag) {
            this->first.flag = flag;
        }

        inline uint8_t get_flag() {
            return this->first.flag;
        }

        inline void set_name_len(uint8_t name_len) {
            this->first.name_len = name_len;
        }

        inline uint8_t get_name_len() {
            return this->first.name_len;
        }

        inline void set_msg_len(uint8_t msg_len) {
            uint8_t res = msg_len & MASK_FIRST_N(MSGR_MSG_LEN_BITS);
            this->first.msg_len_lsb = res & 1;
            this->second.msg_len_rest = res >> 1;
        }

        inline uint8_t get_msg_len() {
            uint8_t res = this->first.msg_len_lsb;
            res |= this->second.msg_len_rest << 1;
            return res;
        }

        inline void set_crc4(uint8_t crc4) {
            this->second.crc4 = crc4;
        }

        inline uint8_t get_crc4() {
            return this->second.crc4;
        }

        // Calculates crc4, replacing crc4 bits with zeroes
        inline uint8_t calculate_crc4() { // Might rename to simple - crc4
            uint8_t res = 0;
            // Not safe in case of concurrent access
            uint8_t tmp_crc4 = this->get_crc4();
            this->set_crc4(0);
            uint8_t *beg = reinterpret_cast<uint8_t *>(this);
            uint8_t *end = beg + sizeof(*this);
            while(beg < end) {
                res = util::crc4(res, *beg++, BITS_PER_BYTE);
            }

            this->set_crc4(tmp_crc4);

            return res;
        }
    };


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
            msg_hdr_t header(FLAG_BITS, msg.name.size(), packet_text_len);
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

            // Modify vector using pointers (not sure if there is other safer way to modify contiguous bytes)
            uint8_t *hdr_beg_ptr = reinterpret_cast<uint8_t *>(&header);
            uint8_t *hdr_end_ptr = hdr_beg_ptr + sizeof(header);
            // Should endianness be really considered?
            if(util::isLittleEndian()) {
                std::copy(hdr_beg_ptr, hdr_end_ptr, res.begin() + header_offset);
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

        msg_hdr_t header;
        uint8_t * const hdr_beg_ptr = reinterpret_cast<uint8_t *>(&header); // Pointer, used to fill header from buffer
        
        if( buff.size() < 2 ) {
            throw std::runtime_error("Buffer does not contain bytes for header (at least 2)");
        }

        // Parse every packet
        std::vector<uint8_t>::iterator cur_iter = buff.begin();
        while(cur_iter != buff.end()) {
            // Copy to struct according to endianness. Byte with flag comes first
            if(util::isLittleEndian()) {
                std::copy(cur_iter, cur_iter + sizeof(header), hdr_beg_ptr);
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

}