file(GLOB AC_LIBRARY_HTTP_UTILS_SOURCES *.cpp)

if (NOT TARGET ac_library_httplike_parser)
    add_subdirectory("../../httplike/parser" ac_library_httplike_parser_bindir)
endif()

add_library(ac_library_http_utils STATIC ${AC_LIBRARY_HTTP_UTILS_SOURCES})

target_link_libraries(
    ac_library_http_utils
    ac_library_httplike_parser
)
