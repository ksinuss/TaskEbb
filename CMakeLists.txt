cmake_minimum_required(VERSION 3.10)

project(final_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_SOURCE_DIR}/doctest
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

add_library(final_project_lib STATIC
    # sources/core/periodic_tracker.cpp
    sources/core/task_template.cpp
    sources/core/task.cpp
)

add_subdirectory(tests)
