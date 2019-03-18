file(GLOB AC_LIBRARY_NET_CLIENT_SOURCES *.cpp)

add_library(ac_library_net_client STATIC ${AC_LIBRARY_NET_CLIENT_SOURCES})

target_link_libraries(
    ac_library_net_client
    ac_common_utils
)
