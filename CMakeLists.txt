cmake_minimum_required (VERSION 3.6)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

project (codecserver-mbelib-module VERSION 0.0.1)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 11)

include(GNUInstallDirs)
find_package(CodecServer REQUIRED)
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
find_package(LibMbe REQUIRED)

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O3 -rdynamic")
SET(CMAKE_C_FLAGS_DEBUG "-g -O3")
SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -rdynamic")
SET(CMAKE_C_FLAGS_RELEASE "-O3")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(src)
