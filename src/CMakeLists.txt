add_subdirectory(game)
add_subdirectory(app)

add_executable(realm main.cpp)
target_link_libraries(realm PRIVATE realm_app)
gamelib_target_warnings(realm)
gamelib_copy_runtime(realm)
