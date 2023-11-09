add_library(utils STATIC naming.cpp)
target_link_libraries(utils PRIVATE fmt::fmt)
target_include_directories(utils PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
