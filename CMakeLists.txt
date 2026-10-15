cmake_minimum_required(VERSION 3.24)

project(GDot
    VERSION 1.0.0
    LANGUAGES CXX
)

if(NOT WIN32)
    message(FATAL_ERROR "G DOT is a Windows application.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)

qt_add_executable(GDot WIN32
    main.cpp
    managed.cpp
    console.cpp
    common.hpp
)

target_link_libraries(GDot PRIVATE
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(GDot PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

if(MSVC)
    target_compile_options(GDot PRIVATE /W4 /utf-8)
endif()

set_target_properties(GDot PROPERTIES
    OUTPUT_NAME "G-DOT"
    WIN32_EXECUTABLE TRUE
)

enable_testing()

qt_add_executable(ProtocolTests
    protocol_tests.cpp
    common.hpp
)

target_link_libraries(ProtocolTests PRIVATE
    Qt6::Widgets
    Qt6::Network
)

target_compile_definitions(ProtocolTests PRIVATE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
)

if(MSVC)
    target_compile_options(ProtocolTests PRIVATE /W4 /utf-8)
endif()

add_test(
    NAME ProtocolTests
    COMMAND ProtocolTests
)

install(
    TARGETS GDot
    RUNTIME DESTINATION .
)

qt_generate_deploy_app_script(
    TARGET GDot
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})