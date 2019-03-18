file(GLOB AC_LIBRARY_HTTP_HANDLER_SOURCES *.cpp)

#if (NOT TARGET ac_library_http)
#    add_subdirectory(".." ac_library_http_bindir)
#endif()

add_library(ac_library_http_handler STATIC ${AC_LIBRARY_HTTP_HANDLER_SOURCES})

add_dependencies(ac_library_http_handler ac_library_http)

target_link_libraries(
    ac_library_http_handler
    ac_library_http
)
