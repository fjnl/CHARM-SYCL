set(headers)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.hpp"
    COMMAND "$<TARGET_FILE:packed_type_gen>" id-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.hpp"
    DEPENDS "$<TARGET_FILE:packed_type_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.hpp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.ipp"
    COMMAND "$<TARGET_FILE:packed_type_gen>" id-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.ipp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.ipp"
    DEPENDS "$<TARGET_FILE:packed_type_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/id.ipp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.hpp"
    COMMAND "$<TARGET_FILE:packed_type_gen>" range-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.hpp"
    DEPENDS "$<TARGET_FILE:packed_type_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.hpp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.ipp"
    COMMAND "$<TARGET_FILE:packed_type_gen>" range-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.ipp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.ipp"
    DEPENDS "$<TARGET_FILE:packed_type_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/range.ipp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    COMMAND "$<TARGET_FILE:item_gen>" item-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp"
    DEPENDS "$<TARGET_FILE:item_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.hpp")

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    COMMAND "$<TARGET_FILE:item_gen>" item-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp"
    DEPENDS "$<TARGET_FILE:item_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/item.ipp")

foreach(type platform device)
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        COMMAND "$<TARGET_FILE:info_gen>" ${type}-decl "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp"
        DEPENDS "$<TARGET_FILE:info_gen>"
    )

    list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.hpp")

    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        COMMAND "$<TARGET_FILE:info_gen>" ${type}-impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp"
        DEPENDS "$<TARGET_FILE:info_gen>"
    )

    list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/${type}_info.ipp")
endforeach()

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sycl/math.ipp"
    COMMAND "$<TARGET_FILE:math_gen>" impl "${CMAKE_CURRENT_BINARY_DIR}/sycl/math.ipp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/sycl/math.ipp"
    DEPENDS "$<TARGET_FILE:math_gen>"
)

list(APPEND headers "${CMAKE_CURRENT_BINARY_DIR}/sycl/math.ipp")

add_library(sycl-headers INTERFACE ${headers})
target_include_directories(sycl-headers INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/../../include" "${CMAKE_CURRENT_BINARY_DIR}/../../include")

install(FILES ${headers} DESTINATION include/charm/sycl)
install(FILES sycl.hpp DESTINATION include/charm)
install(DIRECTORY sycl DESTINATION include/charm)
