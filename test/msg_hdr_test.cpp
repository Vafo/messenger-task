#include "../catch2/catch_amalgamated.hpp"

#include "../include/messenger.hpp"
#include "../include/msg_hdr.hpp"

namespace test {

/**
 * msg_hdr_view_t Unit Tests
*/

TEST_CASE("msg_hdr_view_t: getters on hardcoded header", "[msg_hdr_view_t][normal]") {
    std::array<uint8_t, messenger::detail::HEADER_SIZE> hardcoded_header = {
        0xa5, 0x6f  /*FLAG:0x5 NAME_LEN:4 MSG_LEN:31 CRC4:0x6*/
    };
    const uint8_t flag_real = 0x5;
    const uint8_t name_len_real = 4;
    const uint8_t msg_len_real = 31;
    const uint8_t crc4_real = 0x6;

    // Is there any way to REQUIRE_NOTHROW a declaration of object (?)
    REQUIRE_NOTHROW(messenger::detail::msg_hdr_view_t (hardcoded_header.begin()));
    messenger::detail::msg_hdr_view_t hdr_view(hardcoded_header.begin());

    REQUIRE(hdr_view.get_flag() == flag_real);
    REQUIRE(hdr_view.get_name_len() == name_len_real);
    REQUIRE(hdr_view.get_msg_len() == msg_len_real);
    REQUIRE(hdr_view.get_crc4() == crc4_real);
}

TEST_CASE("msg_hdr_view_t: getters on make_buf", "[msg_hdr_view_t][normal]") {
    const std::string name = "Name";
    const std::string text = "ABCDEFGHIJKLMNOPQRSTUVWXY ?/.\'0";
    const uint8_t crc4_real = 0x6; // hardcoded known crc4
    messenger::msg_t msg(name, text);
    
    std::vector<uint8_t> buf;
    REQUIRE_NOTHROW(buf = messenger::make_buff(msg));

    // check before declaration 
    REQUIRE_NOTHROW(messenger::detail::msg_hdr_view_t (buf.data()));
    messenger::detail::msg_hdr_view_t hdr_view(buf.data());

    REQUIRE(hdr_view.get_flag() == FLAG_BITS);
    REQUIRE(hdr_view.get_name_len() == name.size());
    REQUIRE(hdr_view.get_msg_len() == text.size());
    REQUIRE(hdr_view.get_crc4() == crc4_real);
}

// How to check if function has assert?
// TODO: assert can be hijacked either during compile-time of individual src
// or by separating assert into external object
// which will compile differently either for test-cases or dev/release (additional overhead)
// 
// It is implied that every src is compiled separately
// 
// TEST_CASE("msg_hdr_view_t: passed NULL reference", "[msg_hdr_view_t][false]") {
//     REQUIRE_THROWS( messenger::detail::msg_hdr_view_t (NULL) );
// }


/**
 * msg_hdr_mod_t Unit Tests
*/

TEST_CASE("msg_hdr_mod_t: hardcoded check of setters", "[msg_hdr_mod_t][normal]") {
    std::array<uint8_t, messenger::detail::HEADER_SIZE> hardcoded_header = {
        0xa5, 0x6f  /*FLAG:0x5 NAME_LEN:4 MSG_LEN:31 CRC4:0x6*/
    };
    const uint8_t flag = 0x5;
    const uint8_t name_len = 4;
    const uint8_t msg_len = 31;
    const uint8_t crc4 = 0x6;

    std::array<uint8_t, 2> header;
    // check before declaration
    REQUIRE_NOTHROW(messenger::detail::msg_hdr_mod_t (header.begin()));
    messenger::detail::msg_hdr_mod_t hdr_mod(header.begin(), name_len, msg_len, crc4);

    REQUIRE_THAT(header, Catch::Matchers::RangeEquals(hardcoded_header));
}

} // namespace test