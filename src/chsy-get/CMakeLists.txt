add_executable(chsy-get chsy-get.cpp)
target_link_libraries(chsy-get PRIVATE xcml fmt::fmt pugixml::pugixml cxxopts::cxxopts)
install(TARGETS chsy-get)
