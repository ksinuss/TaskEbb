cmake_minimum_required(VERSION 3.10)
project(final_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SQLite3 REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/doctest
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${SQLite3_INCLUDE_DIRS}
)

add_library(final_project_lib STATIC
    sources/core/task.cpp
    sources/core/task_template.cpp
    sources/core/periodic_tracker.cpp
    sources/core/database_manager.cpp
)

target_link_libraries(final_project_lib PRIVATE ${SQLite3_LIBRARIES})

add_subdirectory(tests)