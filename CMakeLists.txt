cmake_minimum_required(VERSION 3.10)
project(prowiz VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

file(GLOB PROWIZ_LIB_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c
     ${CMAKE_CURRENT_SOURCE_DIR}/src/formats/*.c)

add_library(prowiz_lib STATIC ${PROWIZ_LIB_SOURCES})
set_target_properties(prowiz_lib PROPERTIES OUTPUT_NAME prowiz)
target_include_directories(prowiz_lib
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # The engine relies on 32-bit signed wrap-around, as the original did.
    target_compile_options(prowiz_lib PRIVATE -fwrapv -Wall -Wextra -Wno-unused-parameter)
elseif(MSVC)
    target_compile_definitions(prowiz_lib PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

add_executable(prowiz cli/main.c)
target_link_libraries(prowiz PRIVATE prowiz_lib)
if(MSVC)
    target_compile_definitions(prowiz PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(prowiz_unit_tests tests/unit/unit_tests.c)
    target_link_libraries(prowiz_unit_tests PRIVATE prowiz_lib)
    target_include_directories(prowiz_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    add_test(NAME unit_tests COMMAND prowiz_unit_tests)
endif()

install(TARGETS prowiz prowiz_lib
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib)
install(FILES include/prowiz.h DESTINATION include)
