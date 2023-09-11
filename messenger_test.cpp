#include <iostream>
#include <vector>

#include "messenger.hpp"


int main() {
    messenger::msg_t msg("Vafo", "HELO EVERDAIANE!!1 dwam jdwn aknwkjan dknaw ndkjanw kj nwa");
    std::vector<uint8_t> buff = messenger::make_buff(msg);

    std::cout << std::hex;

    // Byte content of message
    for(std::vector<uint8_t>::iterator iter = buff.begin();
        iter != buff.end(); iter++) {
        std::cout << static_cast<unsigned>(*iter) << " ";
    }
    // How to get back to default state of ostream (?)
    std::cout << std::dec;
    std::cout << std::endl << std::endl;

    // Redundant
    // // Char content of message
    // for(std::vector<uint8_t>::iterator iter = buff.begin();
    //     iter != buff.end(); iter++) {
    //     std::cout << *iter;
    // }
    // std::cout << std::endl << std::endl;

    messenger::msg_t parsed = messenger::parse_buff(buff);

    std::cout << "Name : " << parsed.name << std::endl;
    std::cout << "Text : " << parsed.text << std::endl;

    if(parsed.name == msg.name && parsed.text == parsed.text) {
        std::cout << "Parsed packet is same as original packet" << std::endl;
    }

    // Consider adding custom buffer test

    return 0;
}