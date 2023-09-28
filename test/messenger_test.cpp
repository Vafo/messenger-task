// Poor solution to add all prefixes.
// Need to find way to shorten it and make it independent of code editors
#include <catch2/catch_all.hpp>
#include "messenger.hpp"
#include "msg_hdr.hpp"

#include "test_util.hpp"


namespace test {

const size_t PACKET_SIZE_MAX = messenger::detail::HEADER_SIZE + MSGR_NAME_LEN_MAX + MSGR_MSG_LEN_MAX;

/**
 * make_buf Unit Tests
*/

// Single valid packet
TEST_CASE("make_buf: appropriate name & small msg", "[make_buf][normal]") {    
    // Test inputs
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_res = 
                         util::hardcoded_packet_max_text(test_name, test_text);

    messenger::msg_t msg(test_name, test_text);
    std::vector<uint8_t> res;
    
    REQUIRE( msg.name == test_name );
    REQUIRE( msg.text == test_text );


    REQUIRE_NOTHROW(res = messenger::make_buff(msg));

    REQUIRE_THAT(res, Catch::Matchers::RangeEquals(hardcoded_res));
}

// multiple valid packets
TEST_CASE("make_buf: multiple packets, each having same name & msg", "[make_buf][normal][multiple_packets]") {
    const size_t PACKET_NUM = 5;
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_single_packet = 
                         util::hardcoded_packet_max_text(test_name, test_text);

    // Check if test_text has max size
    REQUIRE(test_text.size() == MSGR_MSG_LEN_MAX);

    std::string input_str = util::repeat_string(test_text, PACKET_NUM);
    messenger::msg_t msg(test_name, input_str);

    const size_t PACKET_SIZE = messenger::detail::HEADER_SIZE + msg.name.size() + test_text.size();

    REQUIRE( msg.name == test_name );
    REQUIRE( msg.text.size() == test_text.size() * PACKET_NUM );
    REQUIRE( msg.text == input_str );

    std::vector<uint8_t> res;

    REQUIRE_NOTHROW(res = messenger::make_buff(msg));

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
    // Test inputs
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_packet = 
                         util::hardcoded_packet_short_text(test_name, test_text);
    
    messenger::msg_t res_msg;
    REQUIRE_NOTHROW(res_msg = messenger::parse_buff(hardcoded_packet));

    REQUIRE(res_msg.name == test_name);
    REQUIRE(res_msg.text == test_text);
}

// Multiple packets
TEST_CASE("parse_buf: multiple valid packets", "[parse_buf][normal]") {
    // Test inputs
    const size_t PACKET_NUM = 5;
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_single_packet = 
                         util::hardcoded_packet_max_text(test_name, test_text);

    // Repeat PACKET_NUM times packet bytes & test_text string
    std::vector<uint8_t> input_vec = util::repeat_vector(hardcoded_single_packet, PACKET_NUM);
    std::string deduced_string = util::repeat_string(test_text, PACKET_NUM);

    messenger::msg_t msg;

    REQUIRE_NOTHROW(msg = messenger::parse_buff(input_vec));

    REQUIRE(msg.name == test_name);
    REQUIRE(msg.text == deduced_string);
}

// Invalid crc4
TEST_CASE("parse_buf: invalid crc4", "[parse_buf][false]") {
    // Test inputs
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_packet = 
                         util::hardcoded_packet_max_text(test_name, test_text);
    
    messenger::msg_t msg;

    SECTION("flipping single bit") {
        // Consider using generators to flip bit in range of bytes
        hardcoded_packet[0] ^= 1;
        REQUIRE_THROWS(msg = messenger::parse_buff(hardcoded_packet));
    }

    SECTION("flipping single byte") {
        hardcoded_packet[0] ^= 0xFF;
        REQUIRE_THROWS(msg = messenger::parse_buff(hardcoded_packet));
    }

}

// Empty buf
TEST_CASE("parse_buf: empty buf", "[parse_buf][false]") {
    std::vector<uint8_t> packet = {/*Empty*/};

    messenger::msg_t msg;
    REQUIRE_THROWS(msg = messenger::parse_buff(packet));
}

// Trimmed buf
TEST_CASE("parse_buf: trimmed buf", "[parse_buf][false]") {
    // Test inputs
    std::string test_name;
    std::string test_text;
    std::vector<uint8_t> hardcoded_packet = 
                         util::hardcoded_packet_max_text(test_name, test_text);

    hardcoded_packet.resize( hardcoded_packet.size() - 1 );

    messenger::msg_t msg;
    REQUIRE_THROWS(msg = messenger::parse_buff(hardcoded_packet));
}

// Non repeating name
TEST_CASE("parse_buf: non repeating name", "[parse_buf][false]") {
    std::string packet1_name;
    std::string packet1_text;
    std::vector<uint8_t> packet1 =
                         util::hardcoded_packet_max_text(packet1_name, packet1_text);
    
    std::string packet2_name;
    std::string packet2_text;
    std::vector<uint8_t> packet2 =
                         util::hardcoded_packet_short_text(packet2_name, packet2_text);

    REQUIRE(packet1_name != packet2_name);

    messenger::msg_t msg;
    
    // Make buf by concatenating packet2 to packet1
    packet1.insert(packet1.end(), packet2.begin(), packet2.end());
    REQUIRE_THROWS(msg = messenger::parse_buff(packet1));

    // Make input buf by placing packet2 | packet1 | packet2
    packet2.insert(packet2.end(), packet1.begin(), packet1.end());
    REQUIRE_THROWS(msg = messenger::parse_buff(packet2));
}


/**
 * make_buf & parse_buf Unit Tests
*/

// Invalid flag, name_len, msg_len and crc4 bit fields
TEST_CASE("make_buf & parse_buf: invalid bit fields", "[make_buf][parse_buf][false]") {
    // Consider using Catch Generators
    // to generate individial and combined cases of invalid bit fields
    const std::string name = "NAMEN";
    const std::string text = "Random.io";
    messenger::msg_t msg(name, text);

    std::vector<uint8_t> packet;
    REQUIRE_NOTHROW(packet = messenger::make_buff(msg));
    messenger::detail::msg_hdr_mod_t hdr_mod(packet.data());

    SECTION("null flag bits") {
        hdr_mod.set_flag(0);
        REQUIRE_THROWS(msg = messenger::parse_buff(packet));
    }
    SECTION("invalid name_len bits") {
        SECTION("nullify name_len") {
            hdr_mod.set_name_len(0);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("increment name_len") {
            hdr_mod.set_name_len(hdr_mod.get_name_len() + 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("decrement name_len") {
            hdr_mod.set_name_len(hdr_mod.get_name_len() - 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }
    }

    SECTION("invalid msg_len bits") {
        SECTION("nullify msg_len") {
            hdr_mod.set_msg_len(0);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("increment msg_len") {
            hdr_mod.set_msg_len(hdr_mod.get_msg_len() + 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("decrement msg_len") {
            hdr_mod.set_msg_len(hdr_mod.get_msg_len() - 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }
    }

    SECTION("invalid crc4 bits") {
        SECTION("nullify crc4") {
            hdr_mod.set_crc4(0);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("increment crc4") {
            hdr_mod.set_crc4(hdr_mod.get_crc4() + 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }

        SECTION("decrement crc4") {
            hdr_mod.set_crc4(hdr_mod.get_crc4() - 1);
            REQUIRE_THROWS(msg = messenger::parse_buff(packet));
        }
    }

}

// 2 valid packets
TEST_CASE("make_buf & parse_buf: appropriate name & large msg, resulting in 2 packets", "[make_buf][parse_buf][normal]") {    
    messenger::msg_t msg("Name", "Lorem ipsum kekus maximus nothing more to say");
    
    REQUIRE( msg.name == "Name" );
    REQUIRE( msg.text == "Lorem ipsum kekus maximus nothing more to say" );

    std::vector<uint8_t> res;

    REQUIRE_NOTHROW(res = messenger::make_buff(msg));
    
    messenger::msg_t parsed_msg;
    REQUIRE_NOTHROW(parsed_msg = messenger::parse_buff(res));
    
    REQUIRE( msg.name == parsed_msg.name );
    REQUIRE( msg.text == parsed_msg.text );
}


} // namespace test