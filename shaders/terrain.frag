#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/tile.glsl"
#include "gamelib/anime_building.glsl"

// Realm's ground and climbables, in the same ink and cel shading as the
// characters:
//   params.x  kind: 0 a ground tile (draw::drawTiles), 1 a floating foothold
//             run, 2 a rope, 3 a ladder, 4 a rope bridge run, 5 a ramp tile
//             rising to +x, 6 a ramp tile rising to -x
//   params.y  material: 0 grass, 1 sand, 2 wood, 3 snow, 4 stone, 5 moss
//   params.z  the run's (or the tile's) world x (patterns stay in the world)
//   color     the cap (grass, sand, snow...), color2 the earth
// Ground tiles are one tile; their outline follows only the open sides
// (tileOpenDistance runs on through connected tiles), so the whole ground
// reads as one rounded shape with a cap along its top. Runs fill their quad:
// a foothold's walkable top is 14 units below the quad's top.

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const int kMatGrass = 0, kMatSand = 1, kMatWood = 2, kMatSnow = 3, kMatStone = 4, kMatMoss = 5;

vec4 groundTile(SpriteData s, vec2 p, int kind, int material)
{
    uint mask = tileMask(s);
    vec2 half_ = s.size * 0.5;
    vec2 world = (vec2(tileCell(s)) + 0.5) * s.size + p;
    AnimeCanvas c = animeBegin(p, 3.0);
    c.shadeDepth = 7.0;
    vec3 earthColor = s.color2.rgb;
    vec3 capColor = s.color.rgb;
    bool ramp = kind == 5 || kind == 6;
    // The ground's distance: the open sides, rounded at outer corners.
    float d;
    float topDepth; // depth below the walking surface (only where it is the top)
    bool hasTop;
    if (ramp) {
        // The slope: rising to +x (kind 5) or to -x (kind 6).
        float sx = kind == 5 ? p.x : -p.x;
        float surface = -sx; // y of the surface at this x (tile-local, y down)
        float dSurf = (surface - p.y) * 0.7071;
        d = max(dSurf, tileOpenDistance(p, mask | kTileN, half_, 14.0));
        topDepth = -dSurf;
        hasTop = true;
    } else {
        d = tileOpenDistance(p, mask, half_, 14.0);
        hasTop = tileOpen(mask, kTileN);
        topDepth = p.y + half_.y;
    }
    AnimeMaterial earth = animeMatte(earthColor);
    animePart(c, d, earth);
    // Texture of the earth: rounded pebbles, a soft band, in world space.
    if (material == kMatStone || material == kMatWood) {
        if (material == kMatStone) {
            animeStones(c, world, d, earth, vec2(30.0, 18.0), 0.0);
        } else {
            animePlanks(c, world, d, earth, 22.0, false, 0.0);
        }
    } else {
        float pebbles = animeSpots(world * vec2(1.0, 1.0), 26.0, 4.2, 3.0);
        animePartNoLine(c, max(pebbles, d + 3.0), animeMatte(mix(earthColor, vec3(1.0), 0.28)));
        float deep = animeSpots(world + vec2(13.0, 7.0), 34.0, 2.6, 9.0);
        animeFlat(c, max(deep, d + 3.0), earth.shade);
    }
    // The cap along the walking surface.
    if (hasTop) {
        float wobble;
        float capH;
        if (material == kMatGrass || material == kMatMoss) {
            capH = material == kMatMoss ? 12.0 : 13.0;
            wobble = abs(sin(world.x * 0.105)) * 6.0; // droopy scallops
        } else if (material == kMatSnow) {
            capH = 15.0;
            wobble = (0.5 + 0.5 * sin(world.x * 0.07 + 1.5 * sin(world.x * 0.021))) * 7.0;
        } else if (material == kMatSand) {
            capH = 11.0;
            wobble = sin(world.x * 0.05) * 2.0;
        } else {
            capH = 9.0;
            wobble = 0.0;
        }
        float capShape = max(d, topDepth - capH - wobble);
        AnimeMaterial cap = material == kMatSnow ? animeMatte(capColor) : animeMaterial(capColor);
        animePart(c, capShape, cap);
        // The light top line.
        animeFlat(c, max(d + 1.2, abs(topDepth - 3.2) - 1.1), mix(capColor, vec3(1.0), 0.42));
        if (material == kMatMoss) {
            // Moss drips.
            float drips = max(abs(mod(world.x, 37.0) - 18.0) - 3.0, topDepth - capH - wobble - 7.0 - 4.0 * hash12(uvec2(uint(floor(world.x / 37.0) + 500.0), 3u)));
            animePart(c, max(max(drips, d), -topDepth + capH - 1.0), cap);
        }
        if (material == kMatSand) {
            float dots = animeSpots(world * 1.3, 12.0, 1.1, 5.0);
            animeFlat(c, max(dots, capShape + 1.0), cap.shade);
        }
    }
    vec4 col = animeFinish(c);
    return col;
}

void main()
{
    SpriteData s = sprite;
    vec2 p = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    int material = int(s.params.y + 0.5);
    float worldX = s.params.z;
    float time = s.time;
    if (kind == 0 || kind == 5 || kind == 6) {
        outColor = groundTile(s, p, kind, material);
        return;
    }
    AnimeCanvas c = animeBegin(p, 3.0);
    vec2 half_ = s.size * 0.5;
    if (kind == 1) {
        // A floating foothold over the whole run.
        float h = 12.0;
        vec2 center = vec2(0.0, -half_.y + 14.0 + h);
        int top = material == kMatWood ? kAnimeTopWood
                : material == kMatSnow ? kAnimeTopSnow
                : material == kMatStone ? kAnimeTopStone : kAnimeTopGrass;
        animeFootholdColored(c, p, center, vec2(half_.x - 12.0, h), top, s.color.rgb, s.color2.rgb, worldX + p.x * 0.0);
    } else if (kind == 2) {
        animeRope(c, p, vec2(0.0, -half_.y), s.size.y, time);
    } else if (kind == 3) {
        animeLadder(c, p, vec2(0.0, -half_.y), s.size.y, 13.0);
    } else if (kind == 4) {
        // A rope bridge: the planks along the walkable line 14 below the top,
        // posts at both ends.
        float y = -half_.y + 16.0;
        vec2 a = vec2(-half_.x + 10.0, y);
        vec2 b = vec2(half_.x - 10.0, y);
        animeRopeBridge(c, p, a, b, 5.0, 12.0);
        AnimeMaterial post = animeMaterial(animeHex(0x9C6438u));
        for (int i = 0; i < 2; ++i) {
            vec2 at = i == 0 ? a : b;
            animePart(c, sdRoundBox(p - at - vec2(0.0, -16.0), vec2(3.0, 22.0), 2.5), post);
        }
    }
    outColor = animeFinish(c);
}
