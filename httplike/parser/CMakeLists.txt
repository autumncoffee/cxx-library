file(GLOB AC_LIBRARY_HTTPLIKE_PARSER_SOURCES *.cpp)

add_library(ac_library_httplike_parser STATIC ${AC_LIBRARY_HTTPLIKE_PARSER_SOURCES})

target_link_libraries(
    ac_library_httplike_parser
    ac_common
    ac_common_utils
)
