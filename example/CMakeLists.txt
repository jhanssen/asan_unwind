set(SOURCES
    main.cpp)

add_executable(asan_unwind_example ${SOURCES})
target_link_libraries(asan_unwind_example asan_unwind)
