set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/headers)
set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sources)

add_library(final_project_lib STATIC
    ${SOURCES_DIR}/core/task.cpp
)

target_include_directories(final_project_lib PUBLIC ${HEADERS_DIR})

add_subdirectory(tests)

cmake_minimum_required(VERSION 3.10)

project(final_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/final_project/doctest)
