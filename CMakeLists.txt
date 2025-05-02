cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0074 NEW)

project(final_project)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.0/mingw_64")
find_package(Qt6 COMPONENTS Widgets REQUIRED)

find_package(SQLite3 REQUIRED)

set(OpenSSL_ROOT "C:/msys64/mingw64")
find_package(OpenSSL 3.0 REQUIRED)

set(ZLIB_ROOT "C:/msys64/mingw64")
find_package(ZLIB REQUIRED)

find_package(CURL REQUIRED)

include_directories(
    ${CURL_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
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
    sources/core/telegram_bot.cpp
    headers/telegram_bot.hpp
    sources/core/telegram_notifier.cpp
    headers/telegram_notifier.hpp
)

target_link_libraries(final_project_lib PRIVATE 
    ${SQLite3_LIBRARIES}
    Qt6::Widgets
    ${CURL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ZLIB_LIBRARIES}
    ws2_32 
    winmm
)

add_executable(TaskEbbGUI 
    sources/main.cpp 
    sources/core/main_window.cpp
    sources/core/task.cpp
    headers/main_window.hpp
)

target_link_libraries(TaskEbbGUI PRIVATE
    final_project_lib
    Qt6::Widgets
    ${CURL_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${ZLIB_LIBRARIES}
    ws2_32 
    winmm
)

qt_add_resources(final_project_lib "sources"
    PREFIX "/"
    FILES
        sources/icons/tasks.png
        sources/icons/stats.png
)

add_subdirectory(tests)