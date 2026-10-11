#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_building.glsl"

// Realm's houses: ten storybook buildings, each its own shape - never a box
// with a door - in the same ink and cel shading as the characters. Curved
// and tiled roofs with chimneys and eaves, round and shuttered windows that
// glow warm at night, round doors, timber, stone, plaster and planks, signs,
// awnings, lanterns, flower boxes, barrels, crates and chimney smoke.
//
// Design space: 128 x 128 units over the quad height, the ground on y = +58.
//   color   wall color          color2  roof color
//   params  x style (0 cottage, 1 shop, 2 inn, 3 warehouse, 4 townhouse,
//           5 tree hut, 6 library tower, 7 armory, 8 dock house, 9 bakery),
//           y night 0..1 (windows and lanterns lit), z wind (smoke drift)
//   bits.x  0-7 seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;

AnimeMaterial gFrame, gStone, gTimber, gDoor, gIron;
float gNight, gTime, gSeed;

void chimney(vec2 p, vec2 at, float h, float wind)
{
    float d = sdRoundBox(p - at, vec2(5.5, h), 2.0);
    animePart(C, d, gStone);
    animeStones(C, p, d, gStone, vec2(5.5, 4.0), gSeed);
    animePart(C, sdRoundBox(p - at - vec2(0.0, -h), vec2(7.0, 2.0), 1.5), gStone);
    animeSmoke(C, p, at + vec2(wind, -h - 3.0), 3.6, gTime, wind);
}

void bracketSign(vec2 p, vec2 wallAt, float dir, int icon, vec3 board)
{
    animePart(C, sdSegment(p, wallAt, wallAt + vec2(dir * 8.0, 0.0), 1.1), gIron);
    animeSign(C, p, wallAt + vec2(dir * 12.0, 0.0), 8.0, icon, animeMaterial(board), gTime);
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    vec2 p = spriteLocal(inUv) / k;
    int style = int(s.params.x + 0.5);
    float night = clamp(s.params.y, 0.0, 1.0);
    float wind = s.params.z;
    float seed = float(s.bits.x & 0xffu);
    float time = s.time;
    vec3 wallColor = s.color.rgb;
    vec3 roofColor = s.color2.rgb;
    gNight = night;
    gTime = time;
    gSeed = seed;

    C = animeBegin(p, 2.4);
    C.shadeDepth = 5.0;
    AnimeMaterial wall = animeMatte(wallColor);
    AnimeMaterial roof = animeMaterial(roofColor);
    gFrame = animeMaterial(animeHex(0xF6F0E2u));
    gStone = animeMatte(animeHex(0xB8B2C6u));
    gTimber = animeMaterial(animeHex(0x8A5A3Cu));
    gDoor = animeMaterial(animeHex(0xA8703Eu));
    gIron = animeMetal(animeHex(0x5E6070u));
    AnimeMaterial shutter = animeMaterial(mix(roofColor, animeHex(0x5CA8C8u), 0.5));
    float ground = 58.0;

    if (style == 0) {
        // ---- A cottage: a puffy thatched roof over round stone walls.
        chimney(p, vec2(-24.0, -18.0), 14.0, wind);
        float wallD = sdRoundBox(p - vec2(0.0, 34.0), vec2(37.0, 24.0), 7.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        float base = max(wallD, -(p.y - 48.0));
        animePartNoLine(C, base, gStone);
        animeStones(C, p, base, gStone, vec2(8.0, 4.5), seed + 1.0);
        animeLine(C, max(abs(p.y - 48.0), wallD), gStone.ink);
        float roofD = sdRoundRoof(p, vec2(0.0, 14.0), 50.0, 44.0, 0.12, 13.0);
        roofD = animeSmoothUnion(roofD, sdEllipse(p - vec2(0.0, -26.0), vec2(14.0, 8.0)), 6.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, 14.0), 50.0, 44.0, 0.12, 13.0), wall);
        animePart(C, roofD, roof);
        animeThatch(C, p, roofD, roof, 7.0, seed);
        animeGloss(C, p, roofD, vec2(-16.0, -14.0), vec2(14.0, 6.0), 0.25);
        animeRoundWindow(C, p, vec2(0.0, -6.0), 5.0, night, gFrame);
        animeWindow(C, p, vec2(-19.0, 44.0), 6.0, 14.0, night, 4.5, gFrame, shutter, true, seed);
        // A round door.
        float doorD = sdCircle(p - vec2(15.0, 47.0), 10.5);
        doorD = max(doorD, p.y - ground + 1.0);
        animePart(C, doorD, gDoor);
        animePlanks(C, p, doorD, gDoor, 4.0, true, seed);
        animePart(C, sdCircle(p - vec2(21.0, 47.0), 1.4), animeMetal(animeHex(0xF4C24Eu)));
        animeFlowerPot(C, p, vec2(31.0, ground), 6.0, animePalette(11u));
        animeFlowerPot(C, p, vec2(-1.0, ground), 5.0, animePalette(4u));
    } else if (style == 1) {
        // ---- A shop: stone ground floor, a big display window under a
        // striped awning, potions on the shelf, a dormer, a hanging sign.
        float wallD = sdRoundBox(p - vec2(0.0, 28.0), vec2(42.0, 29.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        animeTimber(C, p, max(wallD + 1.0, p.y - 26.0), gTimber, 21.0, 0.0, 26.0, 0.0, seed);
        float base = max(wallD, -(p.y - 28.0));
        animePartNoLine(C, base, gStone);
        animeStones(C, p, base, gStone, vec2(10.0, 6.0), seed);
        animeLine(C, max(abs(p.y - 28.0), wallD), gStone.ink);
        float roofD = sdRoundRoof(p, vec2(0.0, 0.0), 52.0, 34.0, 0.35, 7.0);
        float dormer = sdArch(p, vec2(-14.0, -4.0), 9.0, 22.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, 0.0), 52.0, 34.0, 0.35, 7.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.5, 7.5, seed);
        animePart(C, dormer, wall);
        animeRoundWindow(C, p, vec2(-14.0, -10.0), 4.5, night, gFrame);
        vec2 win = vec2(16.0, 38.0);
        float glassFrame = sdRoundBox(p - win, vec2(18.0, 11.0), 3.0);
        if (night > 0.0) {
            C.color = over(vec4(kAnimeWindowLight, glowFalloff(glassFrame, 10.0) * night * 0.5), C.color);
        }
        animePart(C, glassFrame, gFrame);
        float glass = sdRoundBox(p - win, vec2(16.0, 9.0), 2.0);
        animeFlat(C, glass, animeGlass(p, win, 9.0, night));
        for (int i = 0; i < 4; ++i) {
            vec2 b = win + vec2(-11.0 + float(i) * 7.3, 5.0);
            vec3 potion = animePalette(i == 0 ? 2u : (i == 1 ? 8u : (i == 2 ? 5u : 10u)));
            float bottle = min(sdCircle(p - b, 2.8), sdRoundBox(p - b - vec2(0.0, -3.5), vec2(1.0, 1.5), 0.4));
            animePart(C, max(bottle, glass), animeMaterial(potion));
        }
        animeLine(C, max(abs(p.y - win.y - 8.0), glass), gFrame.ink);
        animeAwning(C, p, vec2(16.0, 21.0), 21.0, 10.0, mix(roofColor, animeHex(0xEC5662u), 0.5), animeHex(0xF7F1E3u), 7.0);
        animeDoor(C, p, vec2(-24.0, ground - 1.5), 8.5, 21.0, gDoor, gFrame, night);
        bracketSign(p, vec2(-41.0, 14.0), -1.0, kAnimeIconPotion, animeHex(0xF4EEE0u));
        animeBarrel(C, p, vec2(-47.0, ground), 6.5, 15.0);
        animeLantern(C, p, vec2(-8.0, 18.0), 3.0, night, time);
    } else if (style == 2) {
        // ---- The inn: two timbered floors, a balcony, a tiled roof with
        // two chimneys, lanterns and a mug sign.
        chimney(p, vec2(-28.0, -36.0), 10.0, wind);
        chimney(p, vec2(27.0, -32.0), 9.0, wind * 0.8);
        float wallD = sdRoundBox(p - vec2(0.0, 20.0), vec2(42.0, 37.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        animeTimber(C, p, max(wallD + 1.0, p.y - 34.0), gTimber, 21.0, -14.0, 34.0, 1.0, seed);
        float base = max(wallD, -(p.y - 38.0));
        animePartNoLine(C, base, gStone);
        animeStones(C, p, base, gStone, vec2(9.0, 5.0), seed + 1.0);
        animeLine(C, max(abs(p.y - 38.0), wallD), gStone.ink);
        float roofD = sdRoundRoof(p, vec2(0.0, -12.0), 54.0, 38.0, 0.3, 6.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 6.0), vec2(0.0, -12.0), 54.0, 38.0, 0.3, 6.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 6.0, 8.0, seed);
        animeGloss(C, p, roofD, vec2(-20.0, -38.0), vec2(14.0, 5.0), 0.25);
        animeRoundWindow(C, p, vec2(0.0, -30.0), 4.5, night, gFrame);
        animeWindow(C, p, vec2(-22.0, 14.0), 6.0, 15.0, night, 4.5, gFrame, shutter, true, seed);
        animeWindow(C, p, vec2(22.0, 14.0), 6.0, 15.0, night, 4.5, gFrame, shutter, true, seed + 3.0);
        // The balcony over the door.
        float balcony = sdRoundBox(p - vec2(0.0, 19.0), vec2(12.0, 2.0), 1.5);
        animePart(C, balcony, gTimber);
        for (int i = -2; i <= 2; ++i) {
            animePart(C, sdRoundBox(p - vec2(float(i) * 5.0, 15.0), vec2(0.9, 3.5), 0.6), gTimber);
        }
        animeWindow(C, p, vec2(0.0, 16.0), 6.0, 14.0, night, 0.0, gFrame, shutter, false, seed + 5.0);
        animeDoor(C, p, vec2(-1.0, ground - 1.5), 9.0, 18.0, gDoor, gFrame, night);
        animeWindow(C, p, vec2(-26.0, 52.0), 6.0, 10.0, night, 0.0, gFrame, shutter, true, seed + 7.0);
        animeWindow(C, p, vec2(25.0, 52.0), 6.0, 10.0, night, 0.0, gFrame, shutter, true, seed + 9.0);
        animeLantern(C, p, vec2(-14.0, 32.0), 3.2, night, time);
        animeLantern(C, p, vec2(13.0, 32.0), 3.2, night, time + 1.3);
        bracketSign(p, vec2(41.0, 2.0), 1.0, kAnimeIconMug, animeHex(0xE9C48Au));
    } else if (style == 3) {
        // ---- A warehouse: a tall barn of planks with a big arched cargo
        // door, a hoist beam with a hanging crate, a gambrel roof.
        float wallD = sdRoundBox(p - vec2(0.0, 22.0), vec2(46.0, 36.0), 3.0);
        animePart(C, wallD, wall);
        animePlanks(C, p, wallD, wall, 7.0, true, seed);
        // Gambrel roof: a wide lower slope and a shallow upper cap.
        float roofLow = sdRoundRoof(p, vec2(0.0, -12.0), 54.0, 22.0, 0.62, 4.0);
        float roofTop = sdRoundRoof(p, vec2(0.0, -30.0), 34.0, 16.0, 0.3, 4.0);
        float roofD = min(roofLow, roofTop);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 5.0), vec2(0.0, -12.0), 54.0, 22.0, 0.62, 4.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 6.5, 9.0, seed);
        animeLine(C, max(abs(p.y + 31.0), roofD), roof.ink);
        // The loft door and the hoist.
        float loft = sdRoundBox(p - vec2(0.0, -6.0), vec2(8.0, 8.0), 2.0);
        animePart(C, loft, gDoor);
        animePlanks(C, p, loft, gDoor, 4.0, true, seed);
        animePart(C, sdRoundBox(p - vec2(8.0, -20.0), vec2(16.0, 2.0), 1.5), gTimber);
        animeStroke(C, sdSegment(p, vec2(21.0, -18.0), vec2(21.0, 4.0), 0.0), 0.9, animeHex(0x6A4A2Au));
        animeCrate(C, p, vec2(21.0, 14.0), 9.0);
        // The big arched cargo door, half open (warm light inside at night).
        float arch = sdArch(p, vec2(0.0, ground), 18.0, 34.0);
        animePart(C, arch, gTimber);
        float inside = sdArch(p, vec2(0.0, ground), 15.0, 31.0);
        animeFlat(C, inside, mix(animeHex(0x4A3428u), kAnimeWindowLight * 0.9, night * 0.7));
        float leaf = max(inside, p.x - 1.0);
        animePart(C, leaf, gDoor);
        animePlanks(C, p, leaf, gDoor, 5.0, true, seed + 2.0);
        animeLine(C, max(abs(p.x + 7.0 - (p.y - 40.0) * 0.6), leaf), gDoor.ink);
        // A painted number and crates.
        animeRoundWindow(C, p, vec2(-30.0, 10.0), 5.5, night, gFrame);
        animeRoundWindow(C, p, vec2(30.0, 10.0), 5.5, night, gFrame);
        animeCrate(C, p, vec2(-36.0, ground), 13.0);
        animeCrate(C, p, vec2(-24.0, ground), 11.0);
        animeCrate(C, p, vec2(-30.0, ground - 11.0), 10.0);
        animeBarrel(C, p, vec2(38.0, ground), 6.5, 15.0);
        animeLantern(C, p, vec2(-24.0, 22.0), 3.0, night, time);
    } else if (style == 4) {
        // ---- A townhouse: narrow and tall, window boxes and a steep roof.
        chimney(p, vec2(16.0, -46.0), 8.0, wind);
        float wallD = sdRoundBox(p - vec2(0.0, 14.0), vec2(28.0, 44.0), 3.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        animeTimber(C, p, max(wallD + 1.0, p.y - 46.0), gTimber, 19.0, -26.0, 46.0, 2.0, seed);
        float roofD = sdRoundRoof(p, vec2(0.0, -28.0), 36.0, 34.0, 0.08, 5.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 5.0), vec2(0.0, -28.0), 36.0, 34.0, 0.08, 5.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.5, 7.0, seed);
        animeRoundWindow(C, p, vec2(0.0, -40.0), 4.0, night, gFrame);
        animeWindow(C, p, vec2(-12.0, -2.0), 5.5, 13.0, night, 4.0, gFrame, shutter, true, seed);
        animeWindow(C, p, vec2(12.0, -2.0), 5.5, 13.0, night, 4.0, gFrame, shutter, true, seed + 2.0);
        animeWindow(C, p, vec2(-12.0, 26.0), 5.5, 13.0, night, 4.0, gFrame, shutter, true, seed + 4.0);
        animeWindow(C, p, vec2(12.0, 26.0), 5.5, 13.0, night, 0.0, gFrame, shutter, false, seed + 6.0);
        animeDoor(C, p, vec2(-1.0, ground - 1.5), 8.0, 19.0, gDoor, gFrame, night);
        animeFlowerPot(C, p, vec2(-18.0, ground), 5.0, animePalette(11u));
        animeLantern(C, p, vec2(14.0, 34.0), 3.0, night, time);
    } else if (style == 5) {
        // ---- A tree hut on its branch: a round plank hut, a leaf roof, a
        // little porch with a rope railing, hanging lanterns.
        AnimeMaterial leaves = animeMaterial(roofColor);
        float deck = sdRoundBox(p - vec2(0.0, ground - 3.0), vec2(44.0, 3.5), 3.0);
        AnimeMaterial plank = animeMaterial(animeHex(0xC8955Au));
        float hut = sdRoundBox(p - vec2(-2.0, 34.0), vec2(28.0, 22.0), 12.0);
        animePart(C, hut, wall);
        animePlanks(C, p, hut, wall, 6.0, true, seed);
        // A leafy roof made of big overlapping leaves.
        float roofD = 1e5;
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            vec2 at = vec2(-34.0 + fi * 13.0, 16.0 - sin(fi * 0.55) * 22.0);
            roofD = min(roofD, sdEllipse(rotate2d(p - at, -0.5 + fi * 0.2), vec2(15.0, 9.0)));
        }
        roofD = min(roofD, sdEllipse(p - vec2(-2.0, -6.0), vec2(26.0, 14.0)));
        animeCastShadow(C, hut, roofD - 4.0 + 0.0, wall);
        animePart(C, roofD, leaves);
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            vec2 at = vec2(-34.0 + fi * 13.0, 16.0 - sin(fi * 0.55) * 22.0);
            animeLine(C, max(sdSegment(rotate2d(p - at, -0.5 + fi * 0.2), vec2(-10.0, 0.0), vec2(10.0, 0.0), 0.0), roofD + 1.0), leaves.ink);
        }
        animeGloss(C, p, roofD, vec2(-14.0, -10.0), vec2(14.0, 5.0), 0.3);
        // A sprout of a chimney: a curled stem with a leaf.
        animePart(C, sdLock(p, vec2(8.0, -18.0), vec2(14.0, -28.0), vec2(8.0 + wind * 3.0, -34.0), 2.5, 0.8), leaves);
        animeRoundWindow(C, p, vec2(-16.0, 32.0), 5.0, night, gFrame);
        // A round door.
        float doorD = max(sdCircle(p - vec2(10.0, 46.0), 10.0), p.y - ground + 5.0);
        animePart(C, doorD, gDoor);
        animePlanks(C, p, doorD, gDoor, 4.0, true, seed);
        animePart(C, sdCircle(p - vec2(15.0, 46.0), 1.3), animeMetal(animeHex(0xF4C24Eu)));
        animePart(C, deck, plank);
        animePlanks(C, p, deck, plank, 6.0, true, seed + 2.0);
        AnimeMaterial rope = animeMaterial(animeHex(0xC9A06Au));
        for (int i = -1; i <= 1; i += 2) {
            float x0 = float(i) * 42.0;
            animePart(C, sdRoundBox(p - vec2(x0, ground - 12.0), vec2(1.6, 8.0), 1.2), animeMaterial(animeHex(0x9C6438u)));
            animePart(C, sdRoundBox(p - vec2(float(i) * 30.0, ground - 12.0), vec2(1.6, 8.0), 1.2), animeMaterial(animeHex(0x9C6438u)));
            float t = clamp((p.x - min(x0, float(i) * 30.0)) / 12.0, 0.0, 1.0);
            float sagY = ground - 18.0 + 3.0 * 4.0 * t * (1.0 - t);
            animePart(C, max(abs(p.y - sagY) - 0.9, max(min(x0, float(i) * 30.0) - p.x, p.x - max(x0, float(i) * 30.0))), rope);
        }
        animeLantern(C, p, vec2(26.0, 14.0), 3.0, night, time);
        animeFlowerPot(C, p, vec2(-34.0, ground - 6.0), 4.5, animePalette(11u));
    } else if (style == 6) {
        // ---- The library tower: a round stone tower with a cone roof, a
        // star weathervane, big round windows and a book sign.
        float towerD = sdRoundBox(p - vec2(0.0, 14.0), vec2(24.0, 44.0), 6.0);
        towerD = min(towerD, sdRoundBox(p - vec2(-22.0, 38.0), vec2(20.0, 20.0), 4.0)); // the reading room
        animePart(C, towerD, wall);
        animeStones(C, p, towerD, wall, vec2(10.0, 6.0), seed);
        float roofSide = sdRoundRoof(p, vec2(-24.0, 18.0), 24.0, 14.0, 0.2, 4.0);
        float cone = sdTriangle(p, vec2(-30.0, -30.0), vec2(30.0, -30.0), vec2(4.0, -62.0)) - 3.0;
        float roofD = min(cone, roofSide);
        animeCastShadow(C, towerD, cone - 3.0, wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.0, 6.5, seed);
        animePart(C, sdSegment(p, vec2(4.0, -62.0), vec2(4.0, -66.0), 0.9), gIron);
        animePart(C, sdStar(rotate2d(p - vec2(4.0, -68.0), sin(time) * 0.3), 5, 4.0, 1.8) - 0.3, animeMetal(animeHex(0xF4C24Eu)));
        animeRoundWindow(C, p, vec2(0.0, -16.0), 7.5, night, gFrame);
        animeWindow(C, p, vec2(8.0, 16.0), 6.0, 15.0, night, 0.0, gFrame, shutter, false, seed);
        animeWindow(C, p, vec2(-28.0, 50.0), 5.5, 11.0, night, 4.0, gFrame, shutter, true, seed + 2.0);
        animeDoor(C, p, vec2(6.0, ground - 1.5), 8.5, 20.0, gDoor, gFrame, night);
        bracketSign(p, vec2(24.0, 26.0), 1.0, kAnimeIconStar, animeHex(0xD8D0F4u));
        animeLantern(C, p, vec2(-8.0, 30.0), 3.0, night, time);
    } else if (style == 7) {
        // ---- The armory: thick stone walls, a forge glowing in an open
        // front, a big chimney, a sword sign.
        float stack = sdRoundBox(p - vec2(-26.0, -28.0), vec2(8.0, 22.0), 2.0);
        animePart(C, stack, gStone);
        animeStones(C, p, stack, gStone, vec2(7.0, 4.5), seed);
        // Sparks and smoke from the forge.
        animeSmoke(C, p, vec2(-26.0 + wind, -52.0), 4.4, time, wind);
        float wallD = sdRoundBox(p - vec2(0.0, 28.0), vec2(44.0, 29.0), 4.0);
        animePart(C, wallD, wall);
        animeStones(C, p, wallD, wall, vec2(12.0, 7.0), seed + 1.0);
        float roofD = sdRoundRoof(p, vec2(0.0, 2.0), 54.0, 26.0, 0.4, 6.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 5.0), vec2(0.0, 2.0), 54.0, 26.0, 0.4, 6.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 6.0, 8.5, seed);
        // The open forge with its fire.
        float mouth = sdArch(p, vec2(14.0, ground), 16.0, 26.0);
        animePart(C, mouth, gTimber);
        float inside = sdArch(p, vec2(14.0, ground), 13.0, 23.0);
        float flicker = 0.85 + 0.15 * sin(time * 9.0 + sin(time * 13.0));
        animeFlat(C, inside, mix(animeHex(0x3A2420u), animeHex(0xFF8A3Au), 0.55 * flicker));
        float fire = sdLock(p, vec2(14.0, ground - 2.0), vec2(10.0, ground - 10.0), vec2(14.0 + sin(time * 7.0) * 2.0, ground - 18.0), 7.0, 1.0);
        animeFlat(C, max(fire, inside), animeHex(0xFFD86Au) * 1.3);
        C.color = over(vec4(animeHex(0xFF9A4Au), glowFalloff(inside, 10.0) * 0.4 * flicker), C.color);
        animeWindow(C, p, vec2(-24.0, 44.0), 6.0, 14.0, night, 0.0, gFrame, shutter, false, seed);
        bracketSign(p, vec2(-44.0, 18.0), -1.0, kAnimeIconSword, animeHex(0xD8D8E4u));
        // A rack of swords by the wall.
        for (int i = 0; i < 3; ++i) {
            float x = 36.0 + float(i) * 4.0;
            animePart(C, sdLock(p, vec2(x, ground - 2.0), vec2(x, ground - 12.0), vec2(x, ground - 20.0), 1.6, 0.6), animeMetal(animeHex(0xD2DCEEu)));
        }
    } else if (style == 8) {
        // ---- The harbor office: blue planks, portholes, a bell and a
        // life ring, a curved roof and an anchor sign.
        float wallD = sdRoundBox(p - vec2(0.0, 26.0), vec2(40.0, 31.0), 4.0);
        animePart(C, wallD, wall);
        animePlanks(C, p, wallD, wall, 6.0, false, seed);
        float roofD = sdRoundRoof(p, vec2(0.0, -2.0), 50.0, 26.0, 0.5, 9.0);
        animeCastShadow(C, wallD, sdRoundRoof(p - vec2(2.0, 5.0), vec2(0.0, -2.0), 50.0, 26.0, 0.5, 9.0), wall);
        animePart(C, roofD, roof);
        animeRoofTiles(C, p, roofD, roof, 5.5, 7.5, seed);
        // A little bell tower on top.
        float belfry = sdRoundBox(p - vec2(0.0, -32.0), vec2(7.0, 7.0), 2.0);
        animePart(C, belfry, gFrame);
        animePart(C, sdRoundRoof(p, vec2(0.0, -38.0), 10.0, 9.0, 0.1, 2.0), roof);
        float swing = sin(time * 2.0) * 0.2;
        animePart(C, sdEllipse(rotate2d(p - vec2(0.0, -34.0), swing) - vec2(0.0, 2.0), vec2(3.5, 4.0)), animeMetal(animeHex(0xF4C24Eu)));
        animeRoundWindow(C, p, vec2(-20.0, 18.0), 6.5, night, gFrame);
        animeRoundWindow(C, p, vec2(20.0, 18.0), 6.5, night, gFrame);
        animeDoor(C, p, vec2(0.0, ground - 1.5), 9.0, 21.0, gDoor, gFrame, night);
        // A life ring on the wall.
        float ring = abs(sdCircle(p - vec2(-26.0, 42.0), 6.0)) - 2.2;
        animePart(C, ring, animeMaterial(animeHex(0xF6F2EAu)));
        animeFlat(C, max(ring + 0.5, abs(fract(atan(p.y - 42.0, p.x + 26.0) / 1.5708) - 0.5) - 0.25), animeHex(0xEC5662u));
        bracketSign(p, vec2(40.0, 10.0), 1.0, kAnimeIconStar, animeHex(0xBFE0F4u));
        animeLantern(C, p, vec2(14.0, 34.0), 3.0, night, time);
        animeBarrel(C, p, vec2(-40.0, ground), 6.0, 14.0);
    } else {
        // ---- The bakery: a round dome roof, a striped awning, a bread sign
        // and a warm oven glow in the window.
        chimney(p, vec2(24.0, -24.0), 10.0, wind);
        float wallD = sdRoundBox(p - vec2(0.0, 32.0), vec2(38.0, 26.0), 6.0);
        animePart(C, wallD, wall);
        animePlaster(C, p, wallD, wall, seed);
        float dome = sdEllipse(p - vec2(0.0, 6.0), vec2(44.0, 28.0));
        dome = max(dome, p.y - 10.0);
        animeCastShadow(C, wallD, dome - 4.0, wall);
        animePart(C, dome, roof);
        animeRoofTiles(C, p, dome, roof, 5.5, 8.0, seed);
        animePart(C, sdCircle(p - vec2(0.0, -24.0), 4.0), animeMetal(animeHex(0xF4C24Eu)));
        vec2 win = vec2(-14.0, 38.0);
        float glassFrame = sdArch(p, win + vec2(0.0, 9.0), 13.0, 20.0);
        animePart(C, glassFrame, gFrame);
        float glass = sdArch(p, win + vec2(0.0, 8.0), 11.0, 18.0);
        animeFlat(C, glass, mix(animeGlass(p, win, 9.0, night), animeHex(0xFFC27Au), 0.35));
        for (int i = 0; i < 3; ++i) {
            vec2 b = win + vec2(-6.0 + float(i) * 6.0, 4.0);
            animePart(C, max(sdEllipse(p - b, vec2(3.0, 2.0)), glass), animeMaterial(animeHex(0xE0A050u)));
        }
        animeAwning(C, p, vec2(-14.0, 17.0), 17.0, 8.0, animeHex(0xF89A48u), animeHex(0xFFF6E8u), 6.0);
        animeDoor(C, p, vec2(18.0, ground - 1.5), 8.0, 20.0, gDoor, gFrame, night);
        bracketSign(p, vec2(38.0, 20.0), 1.0, kAnimeIconBread, animeHex(0xF4E2B8u));
        animeFlowerPot(C, p, vec2(-34.0, ground), 5.0, animePalette(4u));
    }

    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}
