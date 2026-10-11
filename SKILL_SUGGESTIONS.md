# Skill suggestions

## Already installed

action-combat-frame-data, action-rpg-progression-design, anime-style-shader-art,
arcade-action-physics, articulated-2d-character-rigs, chiptune-music-sequencing,
chiptune-sfx-synthesis, ci-cross-platform, cmake-build-developer,
cmake-library-consumption, computer-graphics-developer, cozy-sim-design,
cpp-debugging-profiling, cpp-performance-developer, cpp-testing, cpp26-developer,
deterministic-replay-testing, game-audio-developer, game-engine-developer,
game-feel-designer, game-library-extraction, game-persistence,
game-ui-text-and-dialogue, game-visual-design, glfw-platform-developer,
glsl-shader-build-embedding, grid-algorithms-for-games, headless-balance-sweeps,
input-rebinding-ui, learning-mentor, logic-puzzle-demo-players,
maplestory-style-art-direction, metroidvania-world-design, platformer-boss-design,
platformer-controller-feel, platformer-demo-bots, platformer-level-design,
pong-arcade-rules, procedural-dungeon-generation, procedural-landscape-shaders,
puzzle-game-design-undo, retro-crt-screen-shader, sdf-2d-shape-shaders,
shader-developer, software-architecture-developer, stroke-font-design,
technical-docs-writer, text-level-authoring-tools, tilemap-and-camera-2d,
vector-arcade-neon-look, vulkan-app-smoke-testing, vulkan-developer,
vulkan-offscreen-postprocess, vulkan-text-and-ui, win32-game-window-automation,
window-screenshot-bursts, windows-release-packaging

## Suggestions

### rpg-campaign-completion-checks
How to prove a quest-driven RPG can be finished without playing it: a
throwaway headless driver outside the repository that walks every map's
tile graph (floors, one-way platforms, ropes and ladders with their tops
as floors, drops, jump reach from the controller's numbers) from every
portal and reports unreachable portals, NPCs and spawn runs; then plays
each job's campaign with a bot that accepts, does and hands in every
quest in order (grinding with the demo bot for kills and drops, fighting
bosses, teleporting between givers), printing levels, coins and the
minutes of grinding per quest, and failing on stalled chains, missing
offers, unplaced givers and drops from species that never spawn.
Why: Realm's checker found a story that stalled after the third chapter
(a quest nobody offered), unreachable top tiers and climbs hanging out of
reach - none visible in screenshots.

### side-view-water-and-harbors
Water for side-scrolling 2D games drawn in a shader: a band from a
surface line down with layered sine waves, a broken foam crest, an ink
line, sliding reflections near the top, caustics fading with depth and
night tinting; piers as solid planks on posts standing in the water,
boats floating with their keels under the surface (drawn before the
water, the near waves after), far sea and islets on the horizon,
gulls; keeping the camera's floor bounds and the walk intact.
Why: Realm's port town first had its ships on cobblestone; the harbor
had to be worked out from scratch.
