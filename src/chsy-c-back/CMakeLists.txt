add_executable(chsy-c-back chsy-c-back.cpp)
target_link_libraries(chsy-c-back PRIVATE fmt::fmt xcml cxxopts pugixml::pugixml utils-target)
install(TARGETS chsy-c-back)
