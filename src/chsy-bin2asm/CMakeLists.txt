override_linker_command()

add_library(cscc-bin2asm OBJECT chsy-bin2asm.cpp)
target_link_libraries(cscc-bin2asm PRIVATE fmt::fmt utils-errors utils-io)

if(APPLE)
    target_compile_definitions(cscc-bin2asm PRIVATE -DBIN2ASM_USE_MACHO)
endif()

add_executable(chsy-bin2asm chsy-bin2asm.cpp)
target_link_libraries(chsy-bin2asm PRIVATE fmt::fmt utils-errors utils-io Threads::Threads dl)
target_compile_definitions(chsy-bin2asm PRIVATE -DIMPLEMENT_MAIN)

if(APPLE)
    target_compile_definitions(chsy-bin2asm PRIVATE -DBIN2ASM_USE_MACHO)
endif()
