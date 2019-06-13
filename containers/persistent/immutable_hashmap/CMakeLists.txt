file(GLOB AC_LIBRARY_CONTAINERS_PERSISTENT_IMMUTABLE_HASHMAP_SOURCES *.cpp)

if (NOT TARGET ac_library_contrib_murmur)
    add_subdirectory("../../../contrib/murmur/" ac_library_contrib_murmur_bindir)
endif()

add_library(ac_library_persistent_immutable_hashmap STATIC ${AC_LIBRARY_CONTAINERS_PERSISTENT_IMMUTABLE_HASHMAP_SOURCES})

target_link_libraries(
    ac_library_persistent_immutable_hashmap
    ac_common
    ac_library_contrib_murmur
    "-labsl_int128"
)
