#include <iostream>
#include <vector>

#include "messenger.hpp"
#include "util.hpp"

int main() {
    messenger::msg_t msg("Vafo", "HELO EVERDAIANE!!1 dwam jdwn aknwkjan dknaw ndkjanw kj nwa");
    std::vector<uint8_t> buff = messenger::make_buff(msg);

    {
        // How to get back to default state of ostream (?)
        // Ans: use helper class - scoped_hex of ostream's
        messenger::util::scoped_hex tmp_obj(std::cout); // Is name really required?

        // Byte content of message
        for(std::vector<uint8_t>::iterator iter = buff.begin();
            iter != buff.end(); iter++) {
            std::cout << static_cast<unsigned>(*iter) << " ";
        }
    }

    std::cout << std::endl << std::endl;

    messenger::msg_t parsed = messenger::parse_buff(buff);

    std::cout << "Name : " << parsed.name << std::endl;
    std::cout << "Text : " << parsed.text << std::endl;

    if(parsed.name == msg.name && parsed.text == parsed.text) {
        std::cout << "Parsed packet is same as original packet" << std::endl;
    }

    // Consider adding custom buffer test

    return 0;
}