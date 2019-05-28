file(GLOB AC_LIBRARY_WIREDTIGER_SOURCES *.cpp)

add_library(ac_library_wiredtiger STATIC ${AC_LIBRARY_WIREDTIGER_SOURCES})

target_link_libraries(
    ac_library_wiredtiger
    ac_common
    "-lwiredtiger"
)
