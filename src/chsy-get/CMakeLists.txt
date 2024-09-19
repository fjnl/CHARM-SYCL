add_library(cscc-get OBJECT chsy-get.cpp)
target_link_libraries(cscc-get PRIVATE xcml fmt::fmt pugixml::pugixml cxxopts::cxxopts)
