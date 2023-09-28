
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(Messenger messenger.cpp util.cpp)

target_include_directories(Messenger PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Messenger compiler_flags)