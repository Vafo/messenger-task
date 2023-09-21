#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <string>
#include <vector>

namespace test {

namespace util {

std::string repeat_string(const std::string &src, size_t times);

template<typename T>
std::vector<T> repeat_vector(const std::vector<T> &src, size_t times);

} // namespace util

} // namespace test

#endif