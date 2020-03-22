file(GLOB AC_LIBRARY_HTTP_CLIENT_SOURCES *.cpp)

add_library(ac_library_http_client STATIC ${AC_LIBRARY_HTTP_CLIENT_SOURCES})

add_dependencies(ac_library_http_client ac_library_http)

target_link_libraries(
    ac_library_http_client
    ac_library_http_server
    ac_library_net_server
    ac_library_net_client
    ac_library_http
    ac_common
)
