override_linker_command()

add_executable(chsy-c-back chsy-c-back.cpp)
target_link_libraries(chsy-c-back PRIVATE fmt::fmt xcml cxxopts pugixml::pugixml utils-target)
target_compile_definitions(chsy-c-back PRIVATE -DIMPLEMENT_MAIN)

add_library(cscc-cback OBJECT chsy-c-back.cpp)
target_link_libraries(cscc-cback PRIVATE fmt::fmt xcml cxxopts pugixml::pugixml utils-target)
