set(SOURCES
    asan_unwind.cpp)
    # sanitizer_stacktrace.cpp)

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_FLAGS "-fno-omit-frame-pointer -Wall")

add_library(asan_unwind STATIC ${SOURCES})
target_include_directories(asan_unwind INTERFACE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(asan_unwind pthread)

set_property(TARGET asan_unwind PROPERTY POSITION_INDEPENDENT_CODE ON)
