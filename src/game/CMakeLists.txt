# realm_game: the rules - the world's maps, the character (jobs, stats,
# skills, gear), items and loot, monsters and area bosses, the quests and
# conversations, the session that runs a playthrough and the demo bot.
# Depends only on GameLib::core and GameLib::ai: no window, GPU, audio or
# clock, deterministic for the per-tick input.
add_library(realm_game STATIC
    rules.cpp)
target_include_directories(realm_game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(realm_game PUBLIC GameLib::core GameLib::ai)
gamelib_target_warnings(realm_game)
