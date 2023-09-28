#include <catch2/catch_all.hpp>

#include "../include/messenger.hpp"
#include "../include/msg_hdr.hpp"
#include "../include/util.hpp"

#include "test_util.hpp"


namespace test {

/**
 * crc4_packet Unit Tests
*/

TEST_CASE("crc4_packet: calculate crc4 from hardcoded packet", "[crc4_packet][normal]") {
    std::string name;
    std::string text;
    std::vector<uint8_t> packet = util::hardcoded_packet_max_text(name, text);
    messenger::detail::msg_hdr_view_t hdr_view(packet.data());

    REQUIRE(
        hdr_view.get_crc4() == messenger::util::crc4_packet(packet.data(), packet.data() + packet.size())
    );
}

TEST_CASE("crc4_packet: calculate crc4 of buf from make_buf", "[crc4_packet][normal]") {
    messenger::msg_t msg("Name?", "Namenamenaeamenaemaema");
    std::vector<uint8_t> buf;

    REQUIRE_NOTHROW(buf = messenger::make_buff(msg));

    // check before declaration
    REQUIRE_NOTHROW(messenger::detail::msg_hdr_view_t (buf.data()));
    messenger::detail::msg_hdr_view_t hdr_view(buf.data());

    REQUIRE(
        hdr_view.get_crc4() == messenger::util::crc4_packet(buf.data(), buf.data() + buf.size())
    );
}

} // namespace test