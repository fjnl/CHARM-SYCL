enable_asan_if_debug()
add_executable(chsy-parse chsy-parse.cpp)
target_link_libraries(chsy-parse PRIVATE xcml fmt::fmt pugixml::pugixml cxxopts::cxxopts)
install(TARGETS ${app})
