# realm_app: everything around the rules - the GameLib main-loop glue, the
# profile and the character slots, the world view and its shaders, the HUD,
# the windows and conversations, the screens, sounds, music and effects.
add_library(realm_app STATIC
    realm_game.cpp
    shapes.cpp
    sheet.cpp)
target_include_directories(realm_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(realm_app PUBLIC realm_game GameLib::app GameLib::ui GameLib::fx GameLib::ai GameLib::storage)
gamelib_target_warnings(realm_app)
# Realm's own procedural shapes, embedded as SPIR-V (see shapes.hpp).
gamelib_add_shaders(realm_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/avatar.frag)
