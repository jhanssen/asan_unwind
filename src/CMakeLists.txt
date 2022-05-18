set(SOURCES
    asan_unwind.cpp
    sanitizer_stacktrace.cpp)

add_library(asan_unwind STATIC ${SOURCES})
target_include_directories(asan_unwind INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(asan_unwind pthread)
