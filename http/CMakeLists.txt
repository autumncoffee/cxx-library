file(GLOB AC_LIBRARY_HTTP_SOURCES *.cpp)

if (NOT TARGET ac_library_httplike_server)
    add_subdirectory("../httplike/server" ac_library_httplike_server_bindir)
endif()

if (NOT TARGET ac_library_httplike_parser)
    add_subdirectory("../httplike/parser" ac_library_httplike_parser_bindir)
endif()

if (NOT TARGET ac_library_http_server)
    add_subdirectory("server")
endif()

if (NOT TARGET ac_library_http_handler)
    add_subdirectory("handler")
endif()

if (NOT TARGET ac_library_http_router)
    add_subdirectory("router")
endif()

if (NOT TARGET ac_library_http_utils)
    add_subdirectory("utils")
endif()

if (NOT TARGET ac_library_websocket_parser)
    add_subdirectory("../websocket/parser" ac_library_websocket_parser_bindir)
endif()

add_library(ac_library_http STATIC ${AC_LIBRARY_HTTP_SOURCES})

target_link_libraries(
    ac_library_http
    ac_library_httplike_server
    ac_library_httplike_parser
    ac_library_http_server
    ac_library_websocket_parser
    ac_common_utils
    "-lcrypto"
)
