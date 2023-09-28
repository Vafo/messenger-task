#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <string>
#include <vector>

namespace test {

namespace util {

using packet_insert_iter = std::back_insert_iterator<std::vector<uint8_t>>;


std::string repeat_string(const std::string &src, size_t times);

template<typename T>
std::vector<T> repeat_vector(const std::vector<T> &src, size_t times);

/**
 * Hardcoded packet with max length text
 * 
 * @param test_name output packet's sender's name
 * @param test_text output packet's message text
 * 
 * @returns hardcoded packet with message fields:
 * Name: "Name"
 * Text: "ABCDEFGHIJKLMNOPQRSTUVWXY ?/.\'0"
*/
std::vector<uint8_t> hardcoded_packet_max_text(
    std::string &test_name/*out*/,
    std::string &test_text/*out*/
);

/**
 * Hardcoded packet with max length text
 * 
 * @param test_name output packet's sender's name
 * @param test_text output packet's message text
 * 
 * @returns hardcoded packet with message fields:
 * Name: "Eman"
 * Text: "Lorem ipsum"
*/
std::vector<uint8_t> hardcoded_packet_short_text(
    std::string &test_name/*out*/,
    std::string &test_text/*out*/
);

} // namespace util

} // namespace test

#endif