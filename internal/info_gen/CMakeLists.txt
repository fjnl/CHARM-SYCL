override_linker_command()
add_executable(info_gen info_gen.cpp)
target_link_libraries(info_gen PRIVATE fmt::fmt Threads::Threads dl)
