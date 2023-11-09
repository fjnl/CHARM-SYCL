add_library(utils-logging OBJECT logging.cpp)
target_link_libraries(utils-logging PUBLIC fmt::fmt)
target_include_directories(utils-logging PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(DEFINED CHARM_SYCL_ENABLE_LOGGING AND CHARM_SYCL_ENABLE_LOGGING)
    target_compile_definitions(utils-logging PRIVATE -DCHARM_SYCL_ENABLE_LOGGING)
endif()
