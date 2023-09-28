add_executable(messenger_test messenger_test.cpp msg_hdr_test.cpp util_test.cpp
               test_util.cpp)

set_target_properties(messenger_test
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(messenger_test PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(messenger_test compiler_flags)