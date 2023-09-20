#include "../catch2/catch_amalgamated.hpp"

#include "../messenger.hpp"
#include "../msg_hdr.hpp"

#include "test_util.hpp"


namespace test {

const size_t PACKET_SIZE_MAX = messenger::detail::HEADER_SIZE + MSGR_NAME_LEN_MAX + MSGR_MSG_LEN_MAX;

/**
 * make_buf Unit Tests
*/

// Single valid packet
TEST_CASE("make_buf: appropriate name & small msg", "[make_buf][normal]") {    
    messenger::msg_t msg("Name", "Lorem ipsum");
    
    REQUIRE( msg.name == "Name" );
    REQUIRE( msg.text == "Lorem ipsum" );

    std::vector<uint8_t> res;
    std::vector<uint8_t> hardcoded_res = {
        0xa5, 0xd5, 'N', 'a', 'm', 'e', 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm'
    };

    REQUIRE_NOTHROW(res = messenger::make_buff(msg));

    REQUIRE_THAT(res, Catch::Matchers::RangeEquals(hardcoded_res));
}

// 2 valid packets
TEST_CASE("make_buf: appropriate name & large msg, resulting in 2 packets", "[make_buf][normal]") {    
    messenger::msg_t msg("Name", "Lorem ipsum kekus maximus nothing more to say");
    
    REQUIRE( msg.name == "Name" );
    REQUIRE( msg.text == "Lorem ipsum kekus maximus nothing more to say" );

    std::vector<uint8_t> res;
    std::vector<uint8_t> hardcoded_res = {
        0xa5, 0x2f, 0x4e, 0x61, 0x6d, 0x65, 0x4c, 0x6f, 
        0x72, 0x65, 0x6d, 0x20, 0x69, 0x70, 0x73, 0x75, 
        0x6d, 0x20, 0x6b, 0x65, 0x6b, 0x75, 0x73, 0x20, 
        0x6d, 0x61, 0x78, 0x69, 0x6d, 0x75, 0x73, 0x20, 
        0x6e, 0x6f, 0x74, 0x68, 0x69, 0x25, 0x17, 0x4e, 
        0x61, 0x6d, 0x65, 0x6e, 0x67, 0x20, 0x6d, 0x6f, 
        0x72, 0x65, 0x20, 0x74, 0x6f, 0x20, 0x73, 0x61, 
        0x79
    };

    REQUIRE_NOTHROW(res = messenger::make_buff(msg));

    REQUIRE_THAT(res, Catch::Matchers::RangeEquals(hardcoded_res));
}

// multiple valid packets
TEST_CASE("make_buf: multiple packets, each having same name & msg", "[make_buf][normal][multiple_packets]") {
    const size_t PACKET_NUM = 5;
    std::string text_base = "ABCDEFGHIJKLMNOPQRSTUVWXY ?/.\'0"; // Length 31 (NAME_LEN_MAX) (exclude \0 from count)
    std::string input_str = util::repeat_string(text_base, PACKET_NUM);
    
    // Check if text_base has max size
    REQUIRE(text_base.size() == MSGR_MSG_LEN_MAX);

    messenger::msg_t msg("Name", input_str);

    const size_t PACKET_SIZE = messenger::detail::HEADER_SIZE + msg.name.size() + text_base.size();

    REQUIRE(msg.name == "Name");

    // Bug with text size
    REQUIRE(msg.text.size() == text_base.size() * PACKET_NUM);
    REQUIRE( msg.text == input_str ); // Maybe should replace with literal (?)

    std::vector<uint8_t> res;

    REQUIRE_NOTHROW(res = messenger::make_buff(msg));

    std::vector<uint8_t> hardcoded_single_packet = {
        0xa5, 0x6f, 0x4e, 0x61, 0x6d, 0x65, 0x41, 0x42,
        0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a,
        0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52,
        0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x20,
        0x3f, 0x2f, 0x2e, 0x27, 0x30
    };

    REQUIRE(hardcoded_single_packet.size() == PACKET_SIZE);
    REQUIRE(res.size() == PACKET_NUM * PACKET_SIZE);

    // View packet at a time
    std::vector<uint8_t> vector_view;
    for(int i = 0; i < PACKET_NUM; i++) {
        vector_view.clear();
        std::back_insert_iterator<std::vector<uint8_t>> ins_iter(vector_view);
        std::copy(
            res.begin() + i * PACKET_SIZE, /*Beginning of i th packet*/
            res.begin() + (i+1) * PACKET_SIZE, /*End of i th packet*/
            ins_iter
        );
        
        REQUIRE_THAT(vector_view, Catch::Matchers::RangeEquals(hardcoded_single_packet));
    }

}

// Invalid name
TEST_CASE("make_buf: empty name & name exceeding NAME_LEN_MAX", "[make_buf][false]") {
    messenger::msg_t msg_empty("", "Lorem ipsum");

    REQUIRE( msg_empty.name == "" );
    REQUIRE( msg_empty.text == "Lorem ipsum" );

    CHECK_THROWS_AS(messenger::make_buff(msg_empty), std::length_error);

    messenger::msg_t msg_long("TooLongToBeAName", "Lorem ipsum");

    REQUIRE( msg_long.name == "TooLongToBeAName" );
    REQUIRE( msg_long.text == "Lorem ipsum" );

    CHECK_THROWS_AS(messenger::make_buff(msg_long), std::length_error);
    
}

// Empty message
TEST_CASE("make_buf: empty msg", "[make_buf][false]") {
    messenger::msg_t msg_empty("Name", "");

    REQUIRE( msg_empty.name == "Name" );
    REQUIRE( msg_empty.text == "" );

    CHECK_THROWS_AS(messenger::make_buff(msg_empty), std::length_error);
}


/**
 * parse_buf Unit Tests
*/

// Single valid packet
TEST_CASE("parse_buf: single valid packet", "[parse_buf][normal]") {
    std::vector<uint8_t> hardcoded_packet = {
        0xa5, 0xd5, 'N', 'a', 'm', 'e', 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm'
    };

    messenger::msg_t res_msg("", ""); // Empty msg, messenger::msg_t has no default constructor
    REQUIRE_NOTHROW(res_msg = messenger::parse_buff(hardcoded_packet));

    REQUIRE(res_msg.name == "Name");
    REQUIRE(res_msg.text == "Lorem ipsum");
}

} // namespace test