cmake_minimum_required(VERSION 3.10)
project(final_project)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(SQLite3 REQUIRED)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/mingw_64/lib/cmake")
find_package(Qt6 COMPONENTS Widgets REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/doctest
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${SQLite3_INCLUDE_DIRS}
    "C:/Qt/6.9.0/mingw_64/include"
    "C:/Qt/6.9.0/mingw_64/include/QtWidgets"
)

add_library(final_project_lib STATIC
    sources/core/task.cpp
    sources/core/task_template.cpp
    sources/core/periodic_tracker.cpp
    sources/core/database_manager.cpp
    sources/core/main_window.cpp
    headers/main_window.hpp
)

target_link_libraries(final_project_lib PRIVATE 
    ${SQLite3_LIBRARIES}
    Qt6::Widgets
)

add_subdirectory(tests)
add_executable(TaskEbbGUI 
    sources/main.cpp 
    sources/core/main_window.cpp
    sources/core/task.cpp
    headers/main_window.hpp
)

target_link_libraries(TaskEbbGUI PRIVATE
    Qt6::Widgets
    final_project_lib
)