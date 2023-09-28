cmake_minimum_required(VERSION 3.15)

project(Messenger)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_11)

add_subdirectory(src)
add_subdirectory(test)

include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.3.0
)

FetchContent_MakeAvailable(Catch2)

# messenger application
add_executable(messenger_app ${CMAKE_SOURCE_DIR}/src/messenger_app.cpp)
target_include_directories(messenger_app PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(messenger_app Messenger)

# messenger tests
target_link_libraries(messenger_test Messenger Catch2::Catch2WithMain)