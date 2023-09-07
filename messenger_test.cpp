#include <iostream>
#include <vector>

#include "messenger.hpp"


int main() {
    messenger::msg_t msg("Vafo", "HELO EVERIANE!!11 MY NAME SDK NJADNK WAN DKJWNAKJDNWAKJNDKJWAN KDNAW KJNDWK JANKNAKJWD");
    std::vector<uint8_t> buff = messenger::make_buff(msg);

    std::cout << std::hex;

    for(std::vector<uint8_t>::iterator iter = buff.begin();
        iter != buff.end(); iter++) {
        std::cout << static_cast<unsigned>(*iter) << " ";
    }
    // How to get back to default state of ostream (?)
    std::cout << std::dec;
    std::cout << std::endl;

    for(std::vector<uint8_t>::iterator iter = buff.begin();
        iter != buff.end(); iter++) {
        std::cout << *iter;
    }
    std::cout << std::endl;


    return 0;
}