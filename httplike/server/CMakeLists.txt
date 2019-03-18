file(GLOB AC_LIBRARY_HTTPLIKE_SERVER_SOURCES *.cpp)

if (NOT TARGET ac_library_httplike_parser)
    add_subdirectory("../parser" ac_library_httplike_parser_bindir)
endif()

if (NOT TARGET ac_library_net_server)
    add_subdirectory("../../net/server" ac_library_net_server_bindir)
endif()

add_library(ac_library_httplike_server STATIC ${AC_LIBRARY_HTTPLIKE_SERVER_SOURCES})

target_link_libraries(
    ac_library_httplike_server
    ac_library_net_server
    ac_library_httplike_parser
    ac_common
)
