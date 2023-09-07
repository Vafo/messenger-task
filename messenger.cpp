#include <iostream>
#include <bitset>

#include "messenger.hpp"
#include "crc4.hpp"

#define BITS_TO_RANGE(num_bits) (((1 << ((num_bits) - 1)) - 1) | (1 << ((num_bits) - 1)))

#define MSGR_FLAG_BITS 3
#define MSGR_FLAG_MAX BITS_TO_RANGE(MSGR_FLAG_BITS)

#define MSGR_NAME_LEN_BITS 4
#define MSGR_NAME_LEN_MAX BITS_TO_RANGE(MSGR_NAME_LEN_BITS)

#define MSGR_MSG_LEN_BITS 5
#define MSGR_MSG_LEN_MAX BITS_TO_RANGE(MSGR_MSG_LEN_BITS)

#define MSGR_CRC4_BITS 4
#define MSGR_CRC4_MAX BITS_TO_RANGE(MSGR_CRC4_BITS)

// Valid range 1 - 31
#define MASK_FIRST_N(n) (( 1 << (n) ) -  1)

#define DEF_TO_STR(def) #def
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#define ARR_LEN(arr) ((sizeof(arr)) / sizeof(arr[0]))

#define BITS_PER_BYTE 8

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

        // uint8_t calculate_crc4() {
        //     uint16_t check = this->to_uint16();
        //     check &= MSGR_CRC4_MAX << 
        //     return util::crc4(0, check, 12);
        // }

        uint16_t to_uint16() {
            uint16_t res;
            res |= *reinterpret_cast<uint8_t *>(&this->second);
            res <<= BITS_PER_BYTE;
            res |= *reinterpret_cast<uint8_t *>(&this->first);
            return res;
        }
    };

    void test_struct() {
        // msg_hdr_t kek = {
        //     .bits = {
        //         .flag = 4,
        //         .name = 10,
        //         .msg_len = 3,
        //         .crc4 = 1
        //     }
        // };

        msg_hdr_t kek;
        kek.set_flag(3);
        kek.set_name_len(10);
        kek.set_msg_len(3);
        kek.set_crc4(1);

        uint8_t *beg = reinterpret_cast<uint8_t *>(&kek);
        uint8_t *end = beg + sizeof(kek);
        
        while(beg != end) {
            std::cout << std::hex << static_cast<unsigned>(*beg) << ' ';
            beg++;
        }
        
        std::cout << std::endl;
        std::cout << std::dec;

        uint8_t res = kek.calculate_crc4();
        std::cout << "CRC4 custom " << static_cast<unsigned>(res) << std::endl;
        
        res = 0;
        res = util::crc4(res, kek.get_msg_len(), MSGR_MSG_LEN_BITS);
        res = util::crc4(res, kek.get_name_len(), MSGR_NAME_LEN_BITS);
        res = util::crc4(res, kek.get_flag(), MSGR_FLAG_BITS);
        std::cout << "CRC4 handmade " << static_cast<unsigned>(res) << std::endl;

        // for(size_t i = 0; i < ARR_LEN(kek.raw); i++) {
        //     std::cout << std::hex << static_cast<unsigned>(kek.raw[i])  << ' ';
        // }
        // std::cout << std::endl;
    }

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
        int packet_text_len = 0;
        uint8_t crc4_res = 0;
        while(msg_text_len > 0) {
            // Header part of packet
            msg_hdr_t header;
            res.resize(res.size() + sizeof(header));

            // Sender (?) Name part of packet
            for(std::string::size_type i = 0; i < msg.name.size(); i++) {
                crc4_res = util::crc4(crc4_res, msg.name[i], BITS_PER_BYTE);
                res.push_back(msg.name[i]);
            }
            // Text part of packet
            packet_text_len = MIN(msg_text_len, MSGR_MSG_LEN_MAX);

            for(msg_text_offset_end = msg_text_offset + packet_text_len ; 
                msg_text_offset < msg_text_offset_end;
                msg_text_offset++ ) {
                res.push_back( static_cast<uint8_t>(msg.text[msg_text_offset]) );
            }

            msg_text_len -= packet_text_len;
        }

    }
}