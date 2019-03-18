file(GLOB AC_LIBRARY_HTTP_ROUTER_SOURCES *.cpp)

if (NOT TARGET ac_library_http_handler)
    add_subdirectory("../handler" ac_library_http_handler_bindir)
endif()

add_library(ac_library_http_router STATIC ${AC_LIBRARY_HTTP_ROUTER_SOURCES})

target_link_libraries(
    ac_library_http_router
    ac_library_http_handler
    "-lpcrecpp"
)
