# Notes for coding agents

Realm is a game on GameLib (pinned in `CMakeLists.txt`; every pin has a row
in `docs/GAMELIB_REVISION.md`). Games have no tests of their own; the one
CTest is the template's `--frames` smoke run with the validation layer.

- `src/game` links only GameLib::core and GameLib::ai: no window, GPU,
  audio or clock. `src/app` is everything else.
- All visuals are shaders (no image files), all sounds GameLib's synth.
- Zero compiler, shader and validation warnings.
- Small Conventional Commits, files staged explicitly, never build output.
- `FEATURES.md` lists only what works; update it with the code.
- Generic mechanisms belong in GameLib: extract there first (with tests),
  then re-pin here.
