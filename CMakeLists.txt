cmake_minimum_required(VERSION 3.15)

project(Messenger)

add_library(compiler_flags INTERFACE)
target_compile_features(compiler_flags INTERFACE cxx_std_11)

add_subdirectory(src)
add_subdirectory(test)

# messenger application
add_executable(messenger_app ${CMAKE_SOURCE_DIR}/src/messenger_app.cpp)
target_include_directories(messenger_app PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(messenger_app Messenger)

# messenger tests
target_link_libraries(messenger_test Messenger)