#include "../catch2/catch_amalgamated.hpp"

#include "../include/messenger.hpp"
#include "../include/msg_hdr.hpp"
#include "../include/util.hpp"

namespace test {

/**
 * crc4_packet Unit Tests
*/

TEST_CASE("crc4_packet: calculate crc4 from hardcoded packet", "[crc4_packet][normal]") {
    std::vector<uint8_t> packet = {
        0xa5, 0x6f /*crc4:0x6*/, 'N', 'a', 'm', 'e', 'A', 'B',
        'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
        'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
        'S', 'T', 'U', 'V', 'W', 'X', 'Y', ' ',
        '?', '/', '.', '\'', '0'
    };
    const uint8_t crc4_real = 0x6;

    REQUIRE(
        crc4_real == messenger::util::crc4_packet(packet.data(), packet.data() + packet.size())
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