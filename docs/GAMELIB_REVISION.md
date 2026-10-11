# GameLib revision

Realm fetches GameLib from GitHub at a pinned commit (`CMakeLists.txt`,
`FetchContent_Declare(GameLib ... GIT_TAG <hash>)`). Each row is a GameLib
commit the game was built and run with (zero warnings, zero validation
messages); bump the pin only together with a new row.

| Date | GameLib commit | Why |
|---|---|---|
| 2026-10-11 | `e9cc68362345e9defb502456c73c717f6c87c067` (release 0.0.11) | the newest GameLib main when Realm started |
| 2026-10-11 | `ba000e7d10b95753af3370558674ee02ed1773b2` | Realm's first P0 batch: KeyPalette and hotbarKeys (the keybinding window and the quick slots), TextField and onText (the creator's name), animeUiSweep (cooldowns) and boundsAboveLowestFloor (the camera) |
