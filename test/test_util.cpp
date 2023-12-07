#include "test_util.hpp"

#include <string>
#include <vector>

namespace test::util {


std::string repeat_string(const std::string &src, size_t times) {
    std::string out_str;
    
    for(size_t i = 0; i < times; ++i) {
        out_str += src;
    }

    return out_str;
}

template<typename T>
std::vector<T> repeat_vector(const std::vector<T> &src, size_t times) {
    typename std::vector<T> out_vec;

    typename std::back_insert_iterator<std::vector<T>> ins_iter(out_vec);
    for(size_t i = 0; i < times; ++i) {
        std::copy(src.begin(), src.end(), ins_iter);
    }

    return out_vec;
}

// Instantion for uint8_t
template std::vector<uint8_t> repeat_vector<uint8_t>(const std::vector<uint8_t> &src, size_t times);

std::vector<uint8_t> hardcoded_packet_max_text(
    std::string &test_name,
    std::string &test_text
) {
    test_name = "Name";
    // Length 31 (NAME_LEN_MAX) (exclude \0 from count)
    test_text = "ABCDEFGHIJKLMNOPQRSTUVWXY ?/.\'0";
    std::vector<uint8_t> hardcoded_single_packet = {
        0xa5, 0x6f /*header bytes FlAG:0x5 NAME_LEN:0x4 MSG_LEN:0x1f CRC4:0x6*/
    };

    packet_insert_iter hard_iter(hardcoded_single_packet);
    hard_iter = std::copy(test_name.begin(), test_name.end(), hard_iter);
    std::copy(test_text.begin(), test_text.end(), hard_iter);

    return hardcoded_single_packet;
}

std::vector<uint8_t> hardcoded_packet_short_text(
    std::string &test_name,
    std::string &test_text
) {
    test_name = "Eman";
    test_text = "Lorem ipsum";
    std::vector<uint8_t> hardcoded_packet = {
        0xa5, 0x85/*header bytes FlAG:0x5 NAME_LEN:0x4 MSG_LEN:0x0b CRC4:0x8*/
    };
    // Populate hardcoded packet
    packet_insert_iter hard_iter(hardcoded_packet);
    hard_iter = std::copy(test_name.begin(), test_name.end(), hard_iter);
    std::copy(test_text.begin(), test_text.end(), hard_iter);

    return hardcoded_packet;
}

} // namespace test::util