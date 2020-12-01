file(GLOB AC_LIBRARY_MONGO_SOURCES *.cpp)

add_library(ac_library_mongo STATIC ${AC_LIBRARY_MONGO_SOURCES})

target_link_libraries(
    ac_library_mongo
    ac_common
    ac_library_models
    "-lbsoncxx"
)
