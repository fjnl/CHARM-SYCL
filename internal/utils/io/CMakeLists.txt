add_library(
    utils-io
    OBJECT

    exec.cpp
    file_desc.cpp
    file.cpp
    io.cpp
    read_proc_self_exe.cpp
)

target_include_directories(utils-io PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(utils-io PUBLIC Boost::leaf utils-errors PRIVATE fmt::fmt)

if(APPLE)
    target_compile_definitions(utils-io PRIVATE -DHAS_NSGetExecutabePath)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL Linux)
    target_compile_definitions(utils-io PRIVATE -DHAS_PROC_SELF_EXE)
endif()
