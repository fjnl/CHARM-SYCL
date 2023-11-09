set(app chsy-format)
add_executable("${app}" "${app}.cpp")
target_link_libraries("${app}" PRIVATE xcml fmt::fmt pugixml::pugixml cxxopts::cxxopts)
install(TARGETS ${app})
