cmake_minimum_required(VERSION 3.16)
project(usv_backend VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USV_ENABLE_WARNINGS "Enable compiler warnings" ON)
option(USV_BUILD_TESTS "Build backend tests" ON)

add_library(usv_backend_core
    src/BackendEngine.cpp
)

target_include_directories(usv_backend_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(USV_ENABLE_WARNINGS)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(usv_backend_core PRIVATE -Wall -Wextra -Wpedantic)
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(usv_backend_core PRIVATE /W4 /permissive-)
    endif()
endif()

add_executable(backend_app
    src/main.cpp
)

target_link_libraries(backend_app PRIVATE usv_backend_core)

if(USV_BUILD_TESTS)
    enable_testing()

    add_executable(backend_engine_tests
        tests/BackendEngineTests.cpp
    )
    target_link_libraries(backend_engine_tests PRIVATE usv_backend_core)

    add_test(NAME backend_engine_tests COMMAND backend_engine_tests)
endif()

include(GNUInstallDirs)
install(TARGETS backend_app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
