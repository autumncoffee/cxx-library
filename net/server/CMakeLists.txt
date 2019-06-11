file(GLOB AC_LIBRARY_NET_SERVER_SOURCES *.cpp)

if (NOT TARGET ac_library_net_client)
    add_subdirectory("../client" ac_library_net_client_bindir)
endif()

add_library(ac_library_net_server STATIC ${AC_LIBRARY_NET_SERVER_SOURCES})

target_link_libraries(
    ac_library_net_server
    ac_library_net_client
    ac_common
    ac_common_utils
    "-lssl"
    "-lcrypto"
)
