include_directories(${CMAKE_SOURCE_DIR}/doctest)
add_executable(tests task_test.cpp)
target_link_libraries(tests PRIVATE final_project_lib)
add_test(NAME Tests COMMAND tests)