file(GLOB AC_LIBRARY_CONTAINERS_RBTREE_SOURCES *.cpp)

add_library(ac_library_rbtree STATIC ${AC_LIBRARY_CONTAINERS_RBTREE_SOURCES})

target_link_libraries(
    ac_library_rbtree
    ac_common
)
