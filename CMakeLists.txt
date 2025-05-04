cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
cmake_policy(SET CMP0074 NEW)

project(final_project)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

if(DEFINED ENV{QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_DIR}")
endif()
find_package(Qt6 COMPONENTS Widgets Charts REQUIRED)

find_package(SQLite3 REQUIRED)

if(NOT DEFINED OpenSSL_ROOT)
    set(possible_openssl_paths 
        "/usr/local/opt/openssl"    # macOS Homebrew
        "C:/OpenSSL"                # Windows default
        "$ENV{OPENSSL_ROOT_DIR}"    # ENV
        "C:/msys64/mingw64"         # MSYS2
    )
    find_path(OpenSSL_ROOT 
        NAMES include/openssl/ssl.h
        PATHS ${possible_openssl_paths}
    )
endif()
find_package(OpenSSL 3.0 REQUIRED)

if(NOT DEFINED ZLIB_ROOT)
    set(possible_zlib_paths 
        "/usr/local"                # macOS/Linux
        "C:/msys64/mingw64"         # MSYS2
        "$ENV{ZLIB_ROOT}"           # ENV
    )
    find_path(ZLIB_ROOT 
        NAMES include/zlib.h
        PATHS ${possible_zlib_paths}
    )
endif()
find_package(ZLIB REQUIRED)

find_package(CURL REQUIRED)

include_directories(
    ${CMAKE_SOURCE_DIR}/doctest
    ${CMAKE_CURRENT_SOURCE_DIR}/headers
    ${Qt6_DIR}/include
    ${SQLite3_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

add_library(final_project_lib STATIC
    sources/core/task.cpp
    sources/core/task_template.cpp
    sources/core/periodic_tracker.cpp
    sources/core/database_manager.cpp
    sources/core/main_window.cpp
    headers/main_window.hpp
    sources/core/telegram_bot.cpp
    sources/core/telegram_notifier.cpp
)

target_link_libraries(final_project_lib PRIVATE 
    Qt6::Widgets
    Qt6::Charts
    SQLite::SQLite3
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

add_executable(TaskEbbGUI sources/main.cpp)

target_link_libraries(TaskEbbGUI PRIVATE
    final_project_lib
    Qt6::Widgets
    Qt6::Charts
    SQLite::SQLite3
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

qt_add_resources(final_project_lib "sources"
    PREFIX "/"
    FILES
        sources/icons/tasks.png
        sources/icons/stats.png
)

add_subdirectory(tests)