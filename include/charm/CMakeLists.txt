set(headers)

if(NOT CLANG_FORMAT)
    set(CLANG_FORMAT ${CMAKE_COMMAND} -E true)
endif()

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    COMMAND "$<TARGET_FILE:item_gen>" item-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    COMMAND ${CLANG_FORMAT} --style=file -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    DEPENDS "$<TARGET_FILE:item_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    COMMAND "$<TARGET_FILE:item_gen>" item-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    COMMAND ${CLANG_FORMAT} --style=file -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    DEPENDS "$<TARGET_FILE:item_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp")

foreach(type platform device)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        COMMAND "$<TARGET_FILE:info_gen>" ${type}-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        COMMAND ${CLANG_FORMAT} --style=file -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        DEPENDS "$<TARGET_FILE:info_gen>"
    )

    list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        COMMAND "$<TARGET_FILE:info_gen>" ${type}-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        COMMAND ${CLANG_FORMAT} --style=file -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        DEPENDS "$<TARGET_FILE:info_gen>"
    )

    list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp")
endforeach()

foreach(
    py

    math_impl.py
    math.py
    vec_fwd.py
    vec_ops.py
    vec_swizzle.py
)
    cmake_path(REMOVE_EXTENSION py OUTPUT_VARIABLE file)

    if(py STREQUAL math.py)
        set(ext .ipp)
    else()
        set(ext .hpp)
    endif()

    set(file ${file}${ext})

    add_custom_command(
        OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/sycl/${file}
        COMMAND
        ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/sycl/${py} > ${CMAKE_CURRENT_BINARY_DIR}/sycl/${file}
        COMMAND
        ${CLANG_FORMAT} --style=file -i ${CMAKE_CURRENT_BINARY_DIR}/sycl/${file}
        DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/sycl/${py} ${CMAKE_CURRENT_SOURCE_DIR}/sycl/vec_gen.py
    )

    list(APPEND headers ${CMAKE_CURRENT_BINARY_DIR}/sycl/${file})
endforeach()

function(generate input output)
    set(script ${CMAKE_CURRENT_SOURCE_DIR}/sycl/runtime/blas_gen.py)
    set(input ${CMAKE_CURRENT_SOURCE_DIR}/sycl/runtime/${input})
    set(output ${CMAKE_CURRENT_BINARY_DIR}/sycl/runtime/${output})

    add_custom_command(
        OUTPUT
        ${output}
        COMMAND
        env PYTHONPATH=${PROJECT_SOURCE_DIR}/internal/python
        ${Python_EXECUTABLE}
        ${script}
        ${input}
        ${output}
        COMMAND
        ${CLANG_FORMAT} --style=file -i ${output}
        DEPENDS
        ${script}
        ${PROJECT_SOURCE_DIR}/internal/python/template.py
        ${PROJECT_SOURCE_DIR}/internal/python/blas_config.py
        ${input}
    )

    list(APPEND headers ${output})
    set(headers ${headers} PARENT_SCOPE)
endfunction()

generate(blas1.ini blas1.hpp)
generate(blas2.ini blas2.hpp)
generate(blas3.ini blas3.hpp)
generate(lapack.ini lapack.hpp)

add_library(sycl-headers INTERFACE ${headers})
target_include_directories(sycl-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../../include" "${CMAKE_CURRENT_BINARY_DIR}/../../include")
target_include_directories(sycl-headers INTERFACE ${PROJECT_SOURCE_DIR}/generated/include)

if(NOT CSCC_PORTABLE_MODE)
    install(FILES ${headers} DESTINATION include/charm/sycl)
    install(FILES sycl.hpp DESTINATION include/charm)
    install(DIRECTORY sycl DESTINATION include/charm)
endif()
