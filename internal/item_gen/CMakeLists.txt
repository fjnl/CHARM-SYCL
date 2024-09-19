override_linker_command()
add_executable(item_gen item_gen.cpp)
target_link_libraries(item_gen PRIVATE fmt::fmt Threads::Threads dl)
