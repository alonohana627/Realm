#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_building.glsl"

// Realm's decorations - trees, pines, bushes, flowers, rocks, lamps, fences,
// signposts, barrels, crates, a well, benches, stalls, bunting, a fountain,
// the lighthouse, a ship, mooring posts, crystals, torches, lanterns, giant
// mushrooms, snow, banners, an anvil, the giant trees of Mossbough, tents,
// campfires, stalagmites, columns, a shrine door, shells, reeds and the
// windmill - in the characters' ink and cel shading.
//
// Design space: 128 units over the quad height (the width follows the
// quad); things stand on y = +58.
//   color   main color          color2  second color
//   params  x kind (game::DecoKind), y lit 0..1 (lamps, windows, torches),
//           z variant, w seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;

const int kTree = 1, kPine = 2, kBush = 3, kFlowers = 4, kGrass = 5, kRock = 6, kLamp = 7, kFence = 8, kSign = 9, kBarrel = 10,
          kCrate = 11, kFlowerPot = 12, kWell = 13, kBench = 14, kStall = 15, kBunting = 16, kFountain = 17, kLighthouse = 18,
          kBoat = 19, kPost = 20, kCrystal = 21, kTorch = 22, kLantern = 23, kMushroom = 24, kSnowpile = 25, kBanner = 26,
          kAnvil = 27, kGiantTree = 28, kTent = 29, kCampfire = 30, kStalagmite = 31, kColumn = 32, kShrine = 33, kShell = 34,
          kReeds = 35, kWindmill = 36;

void flame(vec2 p, vec2 base, float size, float time, float lit)
{
    float f = sdLock(p, base, base + vec2(-size * 0.3, -size * 0.6), base + vec2(sin(time * 8.0) * size * 0.15, -size * 1.4), size * 0.55, size * 0.05);
    C.color = over(vec4(animeHex(0xFFA040u), glowFalloff(f, size * 1.4) * (0.4 + 0.4 * lit)), C.color);
    animePart(C, f, animeGlow(animeHex(0xFFB040u)));
    float core = sdLock(p, base + vec2(0.0, -size * 0.1), base + vec2(-size * 0.1, -size * 0.5), base + vec2(sin(time * 9.0) * size * 0.1, -size * 0.9),
                        size * 0.28, size * 0.04);
    animeFlat(C, core, animeHex(0xFFF0A0u) * 1.2);
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    vec2 p = spriteLocal(inUv) / k;
    vec2 extent = s.size / k * 0.5;
    int kind = int(s.params.x + 0.5);
    float lit = clamp(s.params.y, 0.0, 1.0);
    int variant = int(s.params.z + 0.5);
    float seed = s.params.w;
    float time = s.time;
    float ground = 58.0;
    vec2 at = vec2(0.0, ground);
    vec3 a = s.color.rgb;
    vec3 b = s.color2.rgb;

    C = animeBegin(p, 2.6);
    C.shadeDepth = 4.0;
    if (kind == kTree) {
        animeTree(C, p, at, 0.92, a, b, variant == 1, time, seed);
    } else if (kind == kPine) {
        animePine(C, p, at, 0.92, a, variant == 1, seed);
    } else if (kind == kBush) {
        animeBush(C, p, at, 1.15, a, b, variant == 1, seed);
    } else if (kind == kFlowers) {
        animeFlowerBed(C, p, at, 1.15, a, b, time, seed);
    } else if (kind == kGrass) {
        animeGrassTuft(C, p, at, 1.3, a, time, seed);
    } else if (kind == kRock) {
        animeRock(C, p, at, 1.4, a, animeHex(0x86D065u), variant == 1, seed);
    } else if (kind == kLamp) {
        AnimeMaterial iron = animeMetal(animeHex(0x4A4B5Cu));
        animePart(C, sdRoundBox(p - vec2(-10.0, ground - 3.0), vec2(9.0, 3.0), 2.0), iron);
        animePart(C, sdSegment(p, vec2(-10.0, ground - 4.0), vec2(-10.0, -48.0), 2.4), iron);
        animePart(C, sdLock(p, vec2(-10.0, -44.0), vec2(4.0, -60.0), vec2(14.0, -44.0), 1.8, 1.4), iron);
        animeLantern(C, p, vec2(14.0, -44.0), 9.0, lit, time);
    } else if (kind == kFence) {
        animeFence(C, p, -extent.x + 4.0, extent.x - 4.0, ground, 44.0, a);
    } else if (kind == kSign) {
        AnimeMaterial wood = animeMaterial(animeHex(0xB07A45u));
        animePart(C, sdRoundBox(p - vec2(0.0, 4.0), vec2(3.5, 54.0), 2.5), wood);
        AnimeMaterial board = animeMaterial(animeHex(0xE9C48Au));
        for (int i = 0; i < 2; ++i) {
            float dir = (i == 0) == (variant % 2 == 0) ? 1.0 : -1.0;
            vec2 c0 = vec2(dir * 14.0, -30.0 + float(i) * 22.0);
            vec2 q = vec2((p.x - c0.x) * dir, p.y - c0.y);
            float arrow = min(sdRoundBox(q - vec2(-4.0, 0.0), vec2(18.0, 7.0), 2.0), sdTriangle(q, vec2(12.0, -9.0), vec2(12.0, 9.0), vec2(24.0, 0.0)));
            animePart(C, arrow, board);
            animeLine(C, max(abs(q.y), max(abs(q.x + 2.0) - 12.0, arrow + 1.5)), board.ink);
        }
    } else if (kind == kBarrel) {
        animeBarrel(C, p, at, 26.0, 64.0);
    } else if (kind == kCrate) {
        animeCrate(C, p, at, 56.0);
    } else if (kind == kFlowerPot) {
        animeFlowerPot(C, p, at, 30.0, a);
    } else if (kind == kWell) {
        animeWell(C, p, at, 1.1, a, animeHex(0xB4B0C4u), seed);
    } else if (kind == kBench) {
        animeBench(C, p, at, 1.4, animeHex(0xB07A45u));
    } else if (kind == kStall) {
        animeMarketStall(C, p, at, 0.85, a, b, seed);
    } else if (kind == kBunting) {
        animeBunting(C, p, vec2(-extent.x + 6.0, -40.0), vec2(extent.x - 6.0, -40.0), 22.0, 8.0, time, seed);
    } else if (kind == kFountain) {
        animeFountain(C, p, at, 0.86, animeHex(0xD8D2C4u), animeHex(0x5CB8E8u), time, seed);
    } else if (kind == kLighthouse) {
        // A striped lighthouse on a rocky base: the lamp room glows and
        // sweeps a beam at night.
        AnimeMaterial white = animeMatte(animeHex(0xF6F2EAu));
        AnimeMaterial red = animeMaterial(animeHex(0xEC5662u));
        animeRock(C, p, vec2(-14.0, ground), 0.9, animeHex(0xA8A4BCu), animeHex(0x86D065u), true, seed);
        float tower = sdTrapezoid(p - vec2(0.0, 6.0), 16.0, 11.0, 52.0) - 1.5;
        animePart(C, tower, white);
        float stripes = max(tower + 0.6, abs(mod(p.y + 6.0, 26.0) - 13.0) - 6.0);
        animePartNoLine(C, stripes, red);
        animeLine(C, max(abs(abs(mod(p.y + 6.0, 26.0) - 13.0) - 6.0), tower), red.ink);
        animeWindow(C, p, vec2(0.0, 30.0), 4.0, 9.0, lit, 0.0, animeMaterial(animeHex(0xF6F0E2u)), red, false, seed);
        float gallery = sdRoundBox(p - vec2(0.0, -46.0), vec2(16.0, 2.0), 1.5);
        animePart(C, gallery, animeMetal(animeHex(0x4A4B5Cu)));
        float room = sdRoundBox(p - vec2(0.0, -53.0), vec2(9.0, 6.0), 2.0);
        animePart(C, room, animeGlow(mix(animeHex(0xFFF2B0u), animeHex(0xFFD86Au), lit)));
        float cap = sdRoundRoof(p, vec2(0.0, -59.0), 11.0, 8.0, 0.1, 2.0);
        animePart(C, cap, red);
        float beamA = time * 0.8;
        vec2 dir = vec2(cos(beamA), sin(beamA) * 0.2);
        vec2 q = p - vec2(0.0, -53.0);
        float along = dot(q, dir);
        float beam = abs(dot(q, vec2(-dir.y, dir.x))) - along * 0.12 - 3.0;
        C.color = over(vec4(animeHex(0xFFF4C0u), (1.0 - smoothstep(0.0, 6.0, beam)) * step(0.0, along) * 0.45 * lit * (1.0 - smoothstep(60.0, 200.0, along))), C.color);
        C.color = over(vec4(animeHex(0xFFF0A0u), glowFalloff(room, 14.0) * (0.3 + 0.6 * lit)), C.color);
    } else if (kind == kBoat) {
        // A small ship at its mooring: a round hull, a mast and two sails,
        // a pennant and portholes; it rocks gently on the water.
        float rock = sin(time * 1.2) * 0.04;
        vec2 q = rotate2d(p - vec2(0.0, 40.0), rock) + vec2(0.0, 40.0) + vec2(0.0, sin(time * 1.2 + 1.0) * 1.5);
        AnimeMaterial hull = animeMaterial(a);
        AnimeMaterial trim = animeMaterial(animeHex(0xF6F2EAu));
        AnimeMaterial sail = animeMatte(animeHex(0xFFF8EEu));
        float hullD = sdRoundBox(q - vec2(0.0, 36.0), vec2(56.0, 12.0), 12.0);
        hullD = max(hullD, -(q.y - 24.0));
        hullD = min(hullD, sdTriangle(q, vec2(50.0, 24.0), vec2(70.0, 18.0), vec2(48.0, 46.0)) - 3.0);
        animePart(C, sdSegment(q, vec2(-6.0, 26.0), vec2(-6.0, -52.0), 2.2), animeMaterial(animeHex(0x9C6438u)));
        float sailA = sdTriangle(q, vec2(-4.0, -48.0), vec2(-4.0, 16.0), vec2(36.0, 16.0)) - 2.0;
        float sailB = sdTriangle(q, vec2(-8.0, -42.0), vec2(-8.0, 12.0), vec2(-42.0, 12.0)) - 2.0;
        animePart(C, sailB, animeTinted(sail, vec3(0.92, 0.92, 0.98)));
        animePart(C, sailA, sail);
        animeFlat(C, max(sailA + 1.0, abs(q.y + 8.0) - 2.5), animeHex(0x6AB8E0u));
        animePart(C, sdLock(q, vec2(-6.0, -52.0), vec2(4.0, -54.0 + sin(time * 4.0)), vec2(14.0, -50.0 + sin(time * 4.0) * 2.0), 3.0, 0.5),
                  animeMaterial(b));
        animePart(C, hullD, hull);
        animePartNoLine(C, max(hullD, abs(q.y - 28.0) - 2.5), trim);
        animeLine(C, max(abs(abs(q.y - 28.0) - 2.5), hullD), hull.ink);
        for (int i = 0; i < 3; ++i) {
            animeRoundWindow(C, q, vec2(-30.0 + float(i) * 20.0, 37.0), 3.5, lit, trim);
        }
        animeGloss(C, q, hullD, vec2(-30.0, 30.0), vec2(16.0, 3.0), 0.3);
    } else if (kind == kPost) {
        // A mooring post with a coil of rope.
        AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
        float post = sdRoundBox(p - vec2(0.0, 34.0), vec2(9.0, 24.0), 6.0);
        animePart(C, post, wood);
        animeLine(C, max(abs(p.x + 3.0), post + 2.0), wood.ink);
        AnimeMaterial rope = animeMaterial(animeHex(0xC9A06Au));
        for (int i = 0; i < 3; ++i) {
            animePart(C, max(abs(sdEllipse(p - vec2(0.0, 24.0 + float(i) * 4.0), vec2(11.0, 3.0))) - 1.4, -p.y + 22.0 + float(i) * 4.0 - 10.0), rope);
        }
        if (variant == 1) {
            animeLantern(C, p, vec2(0.0, 8.0), 6.0, lit, time);
        }
    } else if (kind == kCrystal) {
        // A cluster of glowing crystals.
        vec3 glow = variant == 2 ? animeHex(0xB8E8FFu) : (variant == 1 ? animeHex(0xD08CFFu) : animeHex(0x7FE6FFu));
        AnimeMaterial m = animeMaterial(glow);
        float d = 1e5;
        for (int i = 0; i < 4; ++i) {
            float fi = float(i);
            float x = -18.0 + fi * 12.0;
            float hgt = 28.0 + (i == 1 ? 26.0 : (i == 2 ? 14.0 : 0.0));
            float lean = (fi - 1.5) * 0.18;
            vec2 q = rotate2d(p - vec2(x, ground), lean);
            d = min(d, max(abs(q.x) - 6.0 - q.y * 0.02, -q.y - hgt + abs(q.x) * 0.9));
        }
        d = max(d, p.y - ground);
        float pulse = 0.75 + 0.25 * sin(time * 2.0 + seed);
        C.color = over(vec4(glow * 1.3, glowFalloff(d, 16.0) * 0.5 * pulse), C.color);
        animePart(C, d, m);
        animeFlat(C, max(d + 2.0, abs(p.x + p.y * 0.1 + 6.0) - 1.5), vec3(1.0));
    } else if (kind == kTorch) {
        AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
        animePart(C, sdTrapezoid(p - vec2(0.0, 20.0), 3.5, 6.0, 38.0), wood);
        animePart(C, sdRoundBox(p - vec2(0.0, -20.0), vec2(9.0, 4.0), 2.0), animeMetal(animeHex(0x4A4B5Cu)));
        flame(p, vec2(0.0, -23.0), 16.0, time + seed, lit);
    } else if (kind == kLantern) {
        // A lantern on a short pole (docks, camps, branches).
        AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
        animePart(C, sdSegment(p, vec2(0.0, ground), vec2(0.0, -30.0), 2.4), wood);
        animePart(C, sdSegment(p, vec2(0.0, -30.0), vec2(16.0, -30.0), 2.0), wood);
        animeLantern(C, p, vec2(16.0, -28.0), 8.0, lit, time + seed);
    } else if (kind == kMushroom) {
        // A giant mushroom: a stout stalk and a round cap with spots
        // (variant 2 glows in the dark).
        AnimeMaterial stalk = animeMaterial(animeHex(0xFFF3DCu));
        animePart(C, sdTrapezoid(p - vec2(0.0, 30.0), 13.0, 9.0, 28.0) - 3.0, stalk);
        float cap = max(sdEllipse(p - vec2(0.0, -4.0), vec2(48.0, 34.0)), p.y - 6.0);
        cap = animeSmoothUnion(cap, sdRoundBox(p - vec2(0.0, 4.0), vec2(48.0, 3.0), 3.0), 4.0);
        bool glowing = variant == 2;
        if (glowing) {
            C.color = over(vec4(a * 1.3, glowFalloff(cap, 18.0) * 0.4 * (0.8 + 0.2 * sin(time * 1.5 + seed))), C.color);
        }
        animePart(C, cap, animeMaterial(a));
        float spots = max(animeSpots(p + vec2(seed * 10.0, 0.0), 18.0, 5.0, seed), cap + 3.0);
        animePart(C, spots, glowing ? animeGlow(mix(a, vec3(1.0), 0.6)) : animeMatte(animeHex(0xFFF6E4u)));
        animeGloss(C, p, cap, vec2(-18.0, -20.0), vec2(12.0, 8.0), 0.35);
        animeLine(C, max(abs(p.y - 6.0), cap), animeInkOf(a));
    } else if (kind == kSnowpile) {
        float d = sdEllipse(p - vec2(-14.0, ground - 2.0), vec2(30.0, 18.0));
        d = animeSmoothUnion(d, sdEllipse(p - vec2(18.0, ground - 2.0), vec2(26.0, 14.0)), 8.0);
        d = max(d, p.y - ground);
        animePart(C, d, animeMatte(animeHex(0xF6FAFFu)));
        animeGloss(C, p, d, vec2(-20.0, ground - 14.0), vec2(14.0, 4.0), 0.5);
    } else if (kind == kBanner) {
        // A pole with a waving flag (variant colors).
        vec3 cloth = variant == 1 ? animeHex(0x5CA8FFu) : (variant == 2 ? animeHex(0x7CCB58u) : animeHex(0xEC5662u));
        animePart(C, sdSegment(p, vec2(-14.0, ground), vec2(-14.0, -56.0), 2.2), animeMaterial(animeHex(0x8A5A3Cu)));
        animePart(C, sdCircle(p - vec2(-14.0, -58.0), 3.0), animeMetal(animeHex(0xF4C24Eu)));
        float wave = sin(time * 3.0 + p.x * 0.15) * 3.0;
        float flag = sdRoundBox(p - vec2(4.0, -36.0 + wave * (p.x + 14.0) / 36.0), vec2(18.0, 15.0), 3.0);
        flag = max(flag, -(sdTriangle(p, vec2(22.0, -24.0 + wave), vec2(22.0, -48.0 + wave), vec2(10.0, -36.0 + wave))));
        animePart(C, flag, animeMaterial(cloth));
        animeFlat(C, max(sdStar(p - vec2(2.0, -36.0 + wave * 0.5), 5, 6.0, 2.8), flag + 1.0), animeHex(0xFFF2C8u));
    } else if (kind == kAnvil) {
        AnimeMaterial iron = animeMetal(animeHex(0x5E6070u));
        animePart(C, sdTrapezoid(p - vec2(0.0, 46.0), 18.0, 10.0, 12.0), animeMaterial(animeHex(0x8A5A3Cu)));
        float top = sdRoundBox(p - vec2(0.0, 26.0), vec2(24.0, 6.0), 3.0);
        top = min(top, sdTriangle(p, vec2(20.0, 22.0), vec2(20.0, 30.0), vec2(40.0, 24.0)));
        top = min(top, sdRoundBox(p - vec2(0.0, 34.0), vec2(9.0, 6.0), 2.0));
        animePartShine(C, top, iron, animeCover(C, abs(p.y - 21.0) - 0.8) * 0.8, vec3(1.0));
    } else if (kind == kGiantTree) {
        // A giant tree of Mossbough: a massive trunk with roots and bark,
        // and a huge layered canopy (variant: leaf tone).
        AnimeMaterial bark = animeMaterial(animeHex(0x8E5E3Au));
        AnimeMaterial leaves = animeMaterial(variant == 1 ? animeHex(0x5EB85Au) : animeHex(0x6CC462u));
        float w = extent.x * 0.22;
        float trunk = sdTrapezoid(p - vec2(0.0, 10.0), w * 1.0, w * 0.75, 48.0) - 2.0;
        for (int i = -1; i <= 1; i += 2) {
            trunk = animeSmoothUnion(trunk, sdLock(p, vec2(float(i) * w * 0.6, 40.0), vec2(float(i) * w * 1.3, 52.0),
                                                   vec2(float(i) * w * 2.0, ground), 6.0, 2.0), 4.0);
        }
        // Behind-canopy (darker), trunk, front canopy puffs.
        float back = 1e5;
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            back = min(back, sdCircle(p - vec2((fi - 2.5) * extent.x * 0.3, -48.0 + sin(fi * 2.1 + seed) * 6.0), extent.x * 0.22));
        }
        animePart(C, back, animeTinted(leaves, vec3(0.78, 0.82, 0.9)));
        animePart(C, trunk, bark);
        animeLine(C, max(animeBark(p, 9.0, seed), trunk + 2.0), bark.ink);
        float front = 1e5;
        for (int i = 0; i < 5; ++i) {
            float fi = float(i);
            front = min(front, sdCircle(p - vec2((fi - 2.0) * extent.x * 0.33, -58.0 + cos(fi * 1.7 + seed) * 6.0), extent.x * 0.19));
        }
        animePart(C, front, leaves);
        animeGloss(C, p, front, vec2(-extent.x * 0.3, -66.0), vec2(extent.x * 0.2, 6.0), 0.3);
    } else if (kind == kTent) {
        vec3 cloth = variant == 1 ? animeHex(0x6AB8E0u) : (variant == 2 ? animeHex(0xA8C870u) : animeHex(0xF2A060u));
        float tent = sdTriangle(p, vec2(-46.0, ground), vec2(46.0, ground), vec2(0.0, -10.0)) - 2.0;
        animePart(C, tent, animeMaterial(cloth));
        animeLine(C, max(abs(p.x * 0.5 + p.y * 0.0) - 0.5, tent + 1.0), animeInkOf(cloth));
        float door = sdTriangle(p, vec2(-14.0, ground), vec2(14.0, ground), vec2(0.0, 20.0));
        animeFlat(C, door, mix(animeHex(0x4A3428u), kAnimeWindowLight, lit * 0.7));
        animePart(C, sdSegment(p, vec2(0.0, -10.0), vec2(0.0, -24.0), 1.5), animeMaterial(animeHex(0x8A5A3Cu)));
        animePart(C, sdTriangle(p, vec2(0.0, -24.0), vec2(0.0, -16.0), vec2(10.0, -20.0)), animeMaterial(animeHex(0xEC5662u)));
    } else if (kind == kCampfire) {
        AnimeMaterial wood = animeMaterial(animeHex(0x8A5A3Cu));
        animePart(C, sdSegment(p, vec2(-18.0, ground - 3.0), vec2(16.0, ground - 9.0), 3.5), wood);
        animePart(C, sdSegment(p, vec2(-16.0, ground - 9.0), vec2(18.0, ground - 3.0), 3.5), animeTinted(wood, vec3(0.85)));
        for (int i = 0; i < 6; ++i) {
            animePart(C, sdCircle(p - vec2(-24.0 + float(i) * 9.5, ground - 2.5), 4.5), animeMatte(animeHex(0xA8A4BCu)));
        }
        flame(p, vec2(0.0, ground - 8.0), 22.0, time, 1.0);
    } else if (kind == kStalagmite) {
        float d = sdTriangle(p, vec2(-22.0, ground), vec2(22.0, ground), vec2(4.0, -40.0)) - 4.0;
        d = min(d, sdTriangle(p, vec2(8.0, ground), vec2(34.0, ground), vec2(24.0, 0.0)) - 3.0);
        animePart(C, d, animeMatte(a));
        animeLine(C, max(abs(p.x - 2.0 + p.y * 0.15) - 0.2, d + 3.0), animeInkOf(a));
    } else if (kind == kColumn) {
        // An old column (variant 1: broken).
        AnimeMaterial stone = animeMatte(animeHex(0xC8C2D8u));
        float topY = variant == 1 ? -10.0 : -46.0;
        float shaft = sdRoundBox(p - vec2(0.0, (topY + ground) * 0.5), vec2(13.0, (ground - topY) * 0.5), 2.0);
        if (variant == 1) {
            shaft = max(shaft, topY + 6.0 - abs(sin(p.x * 0.4)) * 6.0 - p.y);
        }
        animePart(C, shaft, stone);
        for (int i = -1; i <= 1; ++i) {
            animeLine(C, max(abs(p.x - float(i) * 6.0), shaft + 2.0), stone.ink);
        }
        animePart(C, sdRoundBox(p - vec2(0.0, ground - 4.0), vec2(18.0, 4.0), 1.5), stone);
        if (variant == 0) {
            animePart(C, sdRoundBox(p - vec2(0.0, topY - 4.0), vec2(18.0, 4.0), 1.5), stone);
        }
        animePartNoLine(C, max(shaft, p.y - topY - 14.0 - sin(p.x * 0.3) * 3.0), animeMaterial(animeHex(0x7CC060u)));
    } else if (kind == kShrine) {
        // A stone arch: the Undercroft's old door (variant 0) or the
        // summit's shrine gate (variant 1).
        AnimeMaterial stone = animeMatte(variant == 1 ? animeHex(0xD8DCEAu) : animeHex(0xB0A898u));
        float arch = sdArch(p, vec2(0.0, ground), 34.0, 76.0);
        float hole = sdArch(p, vec2(0.0, ground), 22.0, 62.0);
        animePart(C, max(arch, -hole), stone);
        animeStones(C, p, max(arch, -hole), stone, vec2(10.0, 7.0), seed);
        if (variant == 0) {
            // A heavy iron-bound door, a gem that glows when it opens.
            AnimeMaterial door = animeMaterial(animeHex(0x6A4A3Au));
            animePart(C, hole + 1.0, door);
            animePlanks(C, p, hole + 1.0, door, 7.0, true, seed);
            for (int i = 0; i < 2; ++i) {
                animePart(C, max(abs(p.y - 20.0 - float(i) * 22.0) - 2.0, hole + 2.0), animeMetal(animeHex(0x4A4B5Cu)));
            }
            float gem = sdCircle(p - vec2(0.0, -8.0), 4.5);
            vec3 gc = mix(animeHex(0x6A6AA0u), animeHex(0xA890FFu), lit);
            C.color = over(vec4(gc * 1.4, glowFalloff(gem, 10.0) * lit * 0.6), C.color);
            animePart(C, gem, animeGlow(gc));
        } else {
            animeFlat(C, hole, mix(animeHex(0xC8E0FFu), animeHex(0xFFFFFFu), 0.5 + 0.5 * sin(time)));
            animePart(C, sdStar(p - vec2(0.0, -26.0), 5, 9.0, 4.0) - 0.5, animeMetal(animeHex(0xF4C24Eu)));
        }
    } else if (kind == kShell) {
        vec3 c0 = variant == 1 ? animeHex(0xF6B8C8u) : animeHex(0xF6DEB2u);
        float d = sdEllipse(p - vec2(0.0, ground - 8.0), vec2(20.0, 12.0));
        d = max(d, p.y - ground);
        animePart(C, d, animeMaterial(c0));
        for (int i = -2; i <= 2; ++i) {
            animeLine(C, max(sdSegment(p, vec2(0.0, ground), vec2(float(i) * 9.0, ground - 18.0), 0.0), d + 1.0), animeInkOf(c0));
        }
        animeGloss(C, p, d, vec2(-6.0, ground - 14.0), vec2(6.0, 3.0), 0.5);
    } else if (kind == kReeds) {
        AnimeMaterial leaf = animeMaterial(animeHex(0x6FAA5Au));
        for (int i = 0; i < 5; ++i) {
            float fi = float(i);
            float x = -24.0 + fi * 12.0;
            float sway = sin(time * 1.5 + fi) * 4.0;
            vec2 tip = vec2(x + sway, -20.0 - fi * 6.0 * (fi < 2.5 ? 1.0 : -1.0) - 10.0);
            animePart(C, sdLock(p, vec2(x, ground), vec2(x + sway * 0.5, 20.0), tip, 2.4, 0.6), leaf);
            if (i % 2 == 0) {
                animePart(C, sdRoundBox(p - tip - vec2(0.0, 8.0), vec2(3.2, 8.0), 3.0), animeMaterial(animeHex(0x9C6438u)));
            }
        }
    } else if (kind == kWindmill) {
        // The windmill on Petalbrook's hill: a round tower, a cap and four
        // turning sails.
        AnimeMaterial wallM = animeMatte(animeHex(0xF6EEDCu));
        AnimeMaterial roofM = animeMaterial(animeHex(0xEC5662u));
        float tower = sdTrapezoid(p - vec2(0.0, 18.0), 30.0, 22.0, 40.0) - 3.0;
        animePart(C, tower, wallM);
        animePlaster(C, p, tower, wallM, seed);
        animeWindow(C, p, vec2(0.0, 10.0), 6.0, 13.0, lit, 4.0, animeMaterial(animeHex(0xF6F0E2u)), animeMaterial(animeHex(0x5CA8C8u)), true, seed);
        animeDoor(C, p, vec2(0.0, ground - 1.0), 8.0, 18.0, animeMaterial(animeHex(0xA8703Eu)), animeMaterial(animeHex(0xF6F0E2u)), lit);
        float cap = sdRoundRoof(p, vec2(0.0, -20.0), 28.0, 24.0, 0.1, 8.0);
        animePart(C, cap, roofM);
        animeRoofTiles(C, p, cap, roofM, 5.5, 7.0, seed);
        vec2 hub = vec2(0.0, -24.0);
        float spin = time * 0.6;
        AnimeMaterial sailM = animeMatte(animeHex(0xFFF8EEu));
        AnimeMaterial frameM = animeMaterial(animeHex(0x9C6438u));
        for (int i = 0; i < 4; ++i) {
            float ang = spin + float(i) * 1.5708;
            vec2 dir = vec2(cos(ang), sin(ang));
            vec2 side = vec2(-dir.y, dir.x);
            vec2 q = vec2(dot(p - hub, dir), dot(p - hub, side));
            float sailD = sdRoundBox(q - vec2(30.0, 6.0), vec2(22.0, 6.5), 2.0);
            animePart(C, sailD, sailM);
            animeLine(C, max(abs(mod(q.x, 8.0) - 4.0) - 0.0, sailD + 1.0), sailM.ink);
            animePart(C, sdSegment(p, hub, hub + dir * 54.0, 1.6), frameM);
        }
        animePart(C, sdCircle(p - hub, 4.0), frameM);
    }
    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}
