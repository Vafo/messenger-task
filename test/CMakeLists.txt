add_executable(messenger_test messenger_test.cpp msg_hdr_test.cpp util_test.cpp
               test_util.cpp ${CMAKE_SOURCE_DIR}/catch2/catch_amalgamated.cpp)

set_target_properties(messenger_test
    PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(messenger_test PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/catch2)
target_link_libraries(messenger_test compiler_flags)