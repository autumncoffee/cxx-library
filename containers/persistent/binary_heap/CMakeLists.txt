file(GLOB AC_LIBRARY_CONTAINERS_PERSISTENT_BINARY_HEAP_SOURCES *.cpp)

add_library(ac_library_persistent_binary_heap STATIC ${AC_LIBRARY_CONTAINERS_PERSISTENT_BINARY_HEAP_SOURCES})

target_link_libraries(
    ac_library_persistent_binary_heap
    ac_common
)
