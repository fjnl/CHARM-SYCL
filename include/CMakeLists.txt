if(NOT CSCC_PORTABLE_MODE)
    install(DIRECTORY CL sycl DESTINATION include)
endif()

add_subdirectory(charm)
