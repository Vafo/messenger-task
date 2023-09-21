#include "test_util.hpp"

#include <string>
#include <vector>

namespace test::util {


std::string repeat_string(const std::string &src, size_t times) {
    std::string out_str;
    
    for(size_t i = 0; i < times; ++i) {
        out_str += src;
    }

    return out_str;
}

template<typename T>
std::vector<T> repeat_vector(const std::vector<T> &src, size_t times) {
    typename std::vector<T> out_vec;

    typename std::back_insert_iterator<std::vector<T>> ins_iter(out_vec);
    for(size_t i = 0; i < times; ++i) {
        std::copy(src.begin(), src.end(), ins_iter);
    }

    return out_vec;
}

// Instantion for uint8_t
template std::vector<uint8_t> repeat_vector<uint8_t>(const std::vector<uint8_t> &src, size_t times);

} // namespace test::util