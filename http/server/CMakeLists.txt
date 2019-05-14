file(GLOB AC_LIBRARY_HTTP_SERVER_SOURCES *.cpp)

if (NOT TARGET ac_library_httplike_server)
    add_subdirectory("../../httplike/server" ac_library_httplike_server_bindir)
endif()

if (NOT TARGET ac_library_websocket_parser)
    add_subdirectory("../../websocket/parser" ac_library_websocket_parser_bindir)
endif()

if (NOT TARGET ac_library_net_server)
    add_subdirectory("../../net/server" ac_library_net_server_bindir)
endif()

if (NOT TARGET ac_library_http_handler)
    add_subdirectory("../handler" ac_library_http_handler_bindir)
endif()

#if (NOT TARGET ac_library_http)
#    add_subdirectory(".." ac_library_http_bindir)
#endif()

add_library(ac_library_http_server STATIC ${AC_LIBRARY_HTTP_SERVER_SOURCES})

add_dependencies(ac_library_http_server ac_library_http)

target_link_libraries(
    ac_library_http_server
    ac_library_httplike_server
    ac_library_websocket_parser
    ac_library_net_server
    ac_library_http_handler
    ac_library_http
    ac_common
)
