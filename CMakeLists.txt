cmake_minimum_required(VERSION 3.5)
project(geobuf)

# set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE
        "Release"
        CACHE STRING "" FORCE)
    message(WARNING "Set build type to default: ${CMAKE_BUILD_TYPE}")
else()
    message(STATUS "Your build type: ${CMAKE_BUILD_TYPE}")
endif()
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og -g")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif()

include(cmake/GitHooks.cmake)
# include(cmake/GeneralConfig.cmake)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")

include(cmake/Utilities.cmake)

include_directories(
    ${PROJECT_SOURCE_DIR}/3rdparties/dbg-macro
    ${PROJECT_SOURCE_DIR}/3rdparties/doctest/doctest/
    ${PROJECT_SOURCE_DIR}/3rdparties/geojson-cpp/include/
    ${PROJECT_SOURCE_DIR}/3rdparties/geometry.hpp/include
    ${PROJECT_SOURCE_DIR}/3rdparties/protozero/include
    ${PROJECT_SOURCE_DIR}/3rdparties/rapidjson/include
    ${PROJECT_SOURCE_DIR}/3rdparties/variant/include
    ${PROJECT_SOURCE_DIR}/src)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 0)
set(PATCH_VERSION 1)
include(cmake/SetEnv.cmake)
setup_git_branch()
setup_git_commit_hash()
set(VERSION_H "${CMAKE_BINARY_DIR}/${PROJECT_NAME}/version.h")
configure_file(${PROJECT_SOURCE_DIR}/version.h.in ${VERSION_H} @ONLY)
install(FILES ${VERSION_H} DESTINATION "${INSTALL_INCLUDE_DIR}/${PROJECT_NAME}")

option(BUILD_SHARED_LIBS "Build shared library." OFF)
set(HEADER_ROOT "${PROJECT_SOURCE_DIR}/src/${PROJECT_NAME}")
file(GLOB_RECURSE HEADERS src/**/*.hpp)
file(GLOB_RECURSE SOURCES src/**/*.cpp)
include(cmake/LibraryConfig.cmake)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
print_all_linked_libraries(${PROJECT_NAME})

add_executable(pbf_decoder 3rdparties/protozero/tools/pbf-decoder.cpp)
target_link_libraries(pbf_decoder ${PROJECT_NAME})

option(BUILD_EXAMPLES "Build examples." ON)
if(BUILD_EXAMPLES)
    file(GLOB SRCS examples/*.cpp)
    foreach(src ${SRCS})
        string(
            REGEX
            REPLACE "(^.*/|.cpp$)"
                    ""
                    exe
                    ${src})
        add_executable(${exe} ${src})
        target_link_libraries(${exe} ${PROJECT_NAME})
    endforeach(src)
endif()

option(BUILD_TESTS "Build tests." ON)
if(BUILD_TESTS)
    file(GLOB SRCS tests/test_*.cpp)
    foreach(src ${SRCS})
        string(
            REGEX
            REPLACE "(^.*/|.cpp$)"
                    ""
                    exe
                    ${src})
        add_executable(${exe} ${src})
        target_link_libraries(${exe} ${PROJECT_NAME})
    endforeach(src)
endif()
add_custom_target(test_all COMMAND make -C .. test_all)

include(cmake/InstallConfig.cmake)

# print_include_directories()

# print_all_variables()
