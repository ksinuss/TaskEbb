cmake_minimum_required(VERSION 3.10)

project(final_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,console")

include_directories(
    ${CMAKE_SOURCE_DIR}/doctest
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
)

add_library(final_project_lib STATIC
    sources/core/periodic_tracker.cpp
    sources/core/task_template.cpp
    sources/core/task.cpp
)

# target_include_directories(final_project_lib PUBLIC ${HEADERS_DIR})

add_subdirectory(tests)
