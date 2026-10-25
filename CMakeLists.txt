cmake_minimum_required(VERSION 3.14)
project(gnss-engine VERSION 0.1.0 LANGUAGES CXX)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Options
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_TOOLS "Build command-line tools" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Library sources
set(GNSS_LIB_SOURCES
    src/ubx/ubx_parser.cpp
    src/ubx/rawx_decoder.cpp
    src/ubx/sfrbx_decoder.cpp
    src/ubx/nav_pvt_decoder.cpp
    src/gnss/signal_registry.cpp
    src/gnss/observation.cpp
    src/gnss/diagnostics.cpp
)

# Create the GNSS engine library
add_library(gnss_engine ${GNSS_LIB_SOURCES})
target_include_directories(gnss_engine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Build command-line tools
if(BUILD_TOOLS)
    add_executable(gnss_inspect src/tools/gnss_inspect.cpp)
    target_link_libraries(gnss_inspect gnss_engine)
    
    add_executable(gnss_decode src/tools/gnss_decode.cpp)
    target_link_libraries(gnss_decode gnss_engine)
    
    add_executable(gnss_parser src/tools/gnss_parser.cpp)
    target_link_libraries(gnss_parser gnss_engine)
endif()

# Build unit tests
if(BUILD_TESTS)
    enable_testing()
    
    # Simple test framework - no external dependencies
    add_executable(test_ubx_parser tests/test_ubx_parser.cpp)
    target_link_libraries(test_ubx_parser gnss_engine)
    add_test(NAME test_ubx_parser COMMAND test_ubx_parser)
    
    add_executable(test_rawx tests/test_rawx.cpp)
    target_link_libraries(test_rawx gnss_engine)
    add_test(NAME test_rawx COMMAND test_rawx)
    
    add_executable(test_sfrbx tests/test_sfrbx.cpp)
    target_link_libraries(test_sfrbx gnss_engine)
    add_test(NAME test_sfrbx COMMAND test_sfrbx)
    
    add_executable(test_signal_registry tests/test_signal_registry.cpp)
    target_link_libraries(test_signal_registry gnss_engine)
    add_test(NAME test_signal_registry COMMAND test_signal_registry)
endif()

# Install targets
install(TARGETS gnss_engine
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/gnss
    DESTINATION include
)

if(BUILD_TOOLS)
    install(TARGETS gnss_inspect gnss_decode gnss_parser
        RUNTIME DESTINATION bin
    )
endif()
