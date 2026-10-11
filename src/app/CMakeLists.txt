# realm_app: everything around the rules - the GameLib main-loop glue, the
# profile and the character slots, the world view and its shaders, the HUD,
# the windows and conversations, the screens, sounds, music and effects.
add_library(realm_app STATIC
    actions.cpp
    dialogue_view.cpp
    effects.cpp
    hud.cpp
    look.cpp
    profile.cpp
    realm_game.cpp
    saves.cpp
    screens.cpp
    shapes.cpp
    sheet.cpp
    sounds.cpp
    ui_kit.cpp
    view.cpp
    windows.cpp)
target_include_directories(realm_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(realm_app PUBLIC realm_game GameLib::app GameLib::ui GameLib::fx GameLib::ai GameLib::storage)
gamelib_target_warnings(realm_app)
# Realm's own procedural shapes, embedded as SPIR-V (see shapes.hpp).
gamelib_add_shaders(realm_app SHADERS
    ${PROJECT_SOURCE_DIR}/shaders/avatar.frag
    ${PROJECT_SOURCE_DIR}/shaders/backdrop.frag
    ${PROJECT_SOURCE_DIR}/shaders/boss.frag
    ${PROJECT_SOURCE_DIR}/shaders/building.frag
    ${PROJECT_SOURCE_DIR}/shaders/deco.frag
    ${PROJECT_SOURCE_DIR}/shaders/fx.frag
    ${PROJECT_SOURCE_DIR}/shaders/monster.frag
    ${PROJECT_SOURCE_DIR}/shaders/terrain.frag
    ${PROJECT_SOURCE_DIR}/shaders/ui.frag)
