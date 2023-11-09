add_executable(
    chsy-lower

    builtin.cpp
    chsy-lower.cpp
    common.cpp
    cpu-c.cpp
    inline.cpp
    math.cpp
    nvidia-cuda.cpp
)
target_link_libraries(chsy-lower PRIVATE xcml fmt::fmt pugixml::pugixml cxxopts::cxxopts utils-target utils-hash)
install(TARGETS chsy-lower)
