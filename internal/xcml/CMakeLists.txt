add_executable(xcml_types_gen xcml_types_gen.cpp spec.cpp)
target_link_libraries(xcml_types_gen PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_types_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_type.hpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_types_gen" "${CMAKE_CURRENT_BINARY_DIR}/xcml_type.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_type.hpp"
    DEPENDS xcml_types_gen
)

add_executable(xcml_types_fwd_gen xcml_types_fwd_gen.cpp spec.cpp)
target_link_libraries(xcml_types_fwd_gen PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_types_fwd_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_type_fwd.hpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_types_fwd_gen" "${CMAKE_CURRENT_BINARY_DIR}/xcml_type_fwd.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_type_fwd.hpp"
    DEPENDS xcml_types_fwd_gen
)

add_executable(xcml_to_xml_gen_cpp xcml_to_xml_gen_cpp.cpp spec.cpp xml_spec.cpp)
target_link_libraries(xcml_to_xml_gen_cpp PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_to_xml_gen_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_to_xml.cpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_to_xml_gen_cpp" "${CMAKE_CURRENT_BINARY_DIR}/xcml_to_xml.cpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_to_xml.cpp"
    DEPENDS xcml_to_xml_gen_cpp
)

add_executable(xcml_from_xml_gen_cpp xcml_from_xml_gen_cpp.cpp spec.cpp xml_spec.cpp)
target_link_libraries(xcml_from_xml_gen_cpp PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_from_xml_gen_cpp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_from_xml.cpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_from_xml_gen_cpp" "${CMAKE_CURRENT_BINARY_DIR}/xcml_from_xml.cpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_from_xml.cpp"
    DEPENDS xcml_from_xml_gen_cpp
)

add_executable(xcml_visitor_gen xcml_visitor_gen.cpp spec.cpp)
target_link_libraries(xcml_visitor_gen PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_visitor_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_visitor.hpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_visitor_gen" "${CMAKE_CURRENT_BINARY_DIR}/xcml_visitor.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_visitor.hpp"
    DEPENDS xcml_visitor_gen
)

add_executable(xcml_func_gen xcml_func_gen.cpp spec.cpp)
target_link_libraries(xcml_func_gen PRIVATE fmt::fmt Threads::Threads)
target_include_directories(xcml_func_gen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../vendor/cpp-peglib)

add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/xcml_func.hpp"
    COMMAND "${CMAKE_CURRENT_BINARY_DIR}/xcml_func_gen" "${CMAKE_CURRENT_BINARY_DIR}/xcml_func.hpp"
    COMMAND "${CLANG_FORMAT}" -i "${CMAKE_CURRENT_BINARY_DIR}/xcml_func.hpp"
    DEPENDS xcml_func_gen
)

add_library(
    xcml
    OBJECT
    copy_node.cpp
    recursive_visitor.cpp
    xcml_from_xml.cpp
    xcml_func.hpp
    xcml_to_xml.cpp
    xcml_type_fwd.hpp
    xcml_type.hpp
    xcml_visitor.hpp
)
target_include_directories(xcml PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(xcml PUBLIC utils PRIVATE fmt::fmt pugixml::pugixml Threads::Threads)
