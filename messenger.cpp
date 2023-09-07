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
        
        // TODO Add constructor
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
        void set_flag(uint8_t flag) {
            this->first.flag = flag;
        }

        uint8_t get_flag() {
            return this->first.flag;
        }

        void set_name_len(uint8_t name_len) {
            this->first.name_len = name_len;
        }

        uint8_t get_name_len() {
            return this->first.name_len;
        }

        void set_msg_len(uint8_t msg_len) {
            std::cout << "MSGLEN " << static_cast<unsigned>(msg_len) << std::endl;
            uint8_t res = msg_len & MASK_FIRST_N(MSGR_MSG_LEN_BITS);
            this->first.msg_len_lsb = res & 1;
            this->second.msg_len_rest = res >> 1;
        }

        uint8_t get_msg_len() {
            uint8_t res = this->first.msg_len_lsb;
            res |= this->second.msg_len_rest << 1;
            return res;
        }

        void set_crc4(uint8_t crc4) {
            this->second.crc4 = crc4;
        }

        uint8_t get_crc4() {
            return this->second.crc4;
        }

        // Calculates crc4, replacing crc4 bits with zeroes
        uint8_t calculate_crc4() { // Might rename to simple - crc4
            uint8_t res = 0;
            // Not safe in case of concurrent access
            uint8_t tmp_crc4 = this->get_crc4();

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

        std::vector<uint8_t> res;

        std::string::size_type msg_text_len = msg.text.size();
        std::string::size_type msg_text_offset = 0;
        std::string::size_type msg_text_offset_end = -1;
        std::vector<uint8_t>::size_type header_offset = 0;
        int packet_text_len = 0;
        uint8_t crc4_res = 0;
        while(msg_text_len > 0) {
            packet_text_len = MIN(msg_text_len, MSGR_MSG_LEN_MAX);

            // Header part of packet
            msg_hdr_t header(FLAG_BITS, msg.name.size(), packet_text_len);
            crc4_res = header.calculate_crc4();
            header_offset = res.size();
            res.resize(res.size() + sizeof(header));

            // Sender (?) Name part of packet
            for(std::string::size_type i = 0; i < msg.name.size(); i++) {
                std::string::value_type val = msg.name[i];
                crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE);
                res.push_back( static_cast<uint8_t>(val) );
            }

            // Text part of packet
            for(msg_text_offset_end = msg_text_offset + packet_text_len ; 
                msg_text_offset < msg_text_offset_end;
                msg_text_offset++ ) {
                std::string::value_type val = msg.text[msg_text_offset];
                crc4_res = util::crc4(crc4_res, val, BITS_PER_BYTE);
                res.push_back( static_cast<uint8_t>(val) );
            }
            
            // Place header into packet with calculated crc4
            header.set_crc4(crc4_res);
            
            uint8_t *hdr_beg_ptr = reinterpret_cast<uint8_t *>(&header);
            uint8_t *hdr_end_ptr = hdr_beg_ptr + sizeof(header);
            if(util::isLittleEndian()) {
                std::copy(hdr_beg_ptr, hdr_end_ptr, res.begin() + header_offset);
            } else {
                std::copy_backward(hdr_beg_ptr, hdr_end_ptr, res.begin() + header_offset);
            }
            
            msg_text_len -= packet_text_len;
        }

        return res;
    }
}