#include "test_util.hpp"

#include <string>

namespace test::util {


std::string repeat_string(const std::string &src, size_t times) {
    std::string out_str;
    
    for(size_t i = 0; i < times; ++i) {
        out_str += src;
    }

    return out_str;
}

} // namespace test::util