set(HEADERS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/headers)
set(SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/sources)

add_library(final_project_lib STATIC
    ${SOURCES_DIR}/core/task.cpp
)

target_include_directories(final_project_lib PUBLIC ${HEADERS_DIR})

add_subdirectory(tests)