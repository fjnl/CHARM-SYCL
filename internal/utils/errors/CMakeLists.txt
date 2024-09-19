add_library(
    utils-errors
    OBJECT

    load_stacktrace.cpp
    report_and_exit.cpp
)

target_link_libraries(
    utils-errors
    PUBLIC
    Boost::stacktrace_basic
    Boost::leaf
    PRIVATE
    fmt::fmt
)

target_include_directories(utils-errors PUBLIC include)
target_compile_definitions(utils-errors PRIVATE -DBOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)
