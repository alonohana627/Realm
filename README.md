# Realm

A small MapleStory-inspired open-world side-scrolling RPG on
[GameLib](https://github.com/alonohana627/GameLib): original chibi heroes,
townsfolk and monsters, everything drawn by shaders and every sound
synthesized. Make a character, grow from beginner to warrior or mage on
Petal Isle, sail to Saltwhistle, climb through Fernwood's tree-top village
and Windcrest Pass, and go down into the Undercroft to bring the beacon's
light back.

## Controls (default; every key can be rebound in game with B)

| Key | Action |
|---|---|
| Arrows | walk, climb ropes and ladders, Up at a portal or a person, Down to lie prone or drop through |
| Space / Left Alt | jump (jump again in the air: hyper jump, Up for straight up) |
| Left Ctrl / X | attack |
| Z | pick up |
| Shift, A S D F G H | skills (job skills appear at level 10) |
| 1-6, 0 | potions and the return scroll |
| C K I E Q W M B | stats, skills, items, equipment, quests, world map, minimap size, key bindings |
| Escape | pause and settings; F11 fullscreen |

Gamepads work for movement, attacks and menus.

## Build and run

Requirements are GameLib's (C++26 Clang, CMake 3.28+, Ninja, Vulkan 1.3,
glslc). GameLib is fetched from GitHub at the commit pinned in
`CMakeLists.txt` (see `docs/GAMELIB_REVISION.md`).

```powershell
cmake --preset debug
cmake --build --preset debug
build/debug/bin/realm.exe          # --demo: the demo bot plays
ctest --preset debug               # smoke test: 120 frames with the validation layer
```

Saves (three character slots, each with its own key bindings) and
`realm.ini` live in `%APPDATA%\Realm`; `REALM_DATA_DIR` moves them.

## Developer scenes

`REALM_SCENE` jumps somewhere: `sheet0`-`sheet2` (the character
construction sheets), `title`, `select`, `create`, or a comma list that
starts a fresh character in slot 3, e.g. `map11,level20,warrior,gear,learn,night,stats`
(tokens: `mapN`, `levelN`, `warrior`/`mage`, `gear`, `learn`, `questN`,
`xN`, `night`, `dusk`, `bosslow`, `stats`, `skills`, `inventory`,
`equipment`, `quests`, `worldmap`, `keys`, `talkN`, `voyage`, `ending`).

## Layout

- `src/game` - the rules (GameLib::core only): maps, character, items,
  skills, monsters, bosses, story and quests, the session, the demo bot.
- `src/app` - presentation: the world view, HUD, windows, conversations,
  screens, sounds and music, saves, the main loop glue.
- `shaders` - the avatar, monsters, bosses, buildings, props, terrain and
  water, backdrops, effects and the UI.
