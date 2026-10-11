#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"

// Realm's monsters: ten rounded, cute archetypes with their own silhouettes
// and the same construction as the hero - the face kit's anime eyes (iris,
// pupil, two highlights, blinking), ink lines and one-tone cel shading -
// sharing gamelib/anime_monster.glsl's bouncy states (idle, move, wind-up,
// hit, defeat). A variant bit turns an archetype into its cousin (a glowing
// cap, an ice jelly, a rune shell, a woolly hog, a leafy wisp). Elites keep
// the silhouette and add a crown and an aura (drawn larger by the game).
//
// Design space: 128 x 128 units over the quad height, feet on y = +50,
// facing +x.
//   color   body color          color2  accent (cap, shell, claws, wings)
//   params  x kind (0 bellcap, 1 mochi, 2 turbanshell, 3 crab, 4 gull,
//           5 pudgy, 6 stumpling, 7 wisp, 8 pebblet, 9 flitter), y state (0
//           idle, 1 move, 2 wind-up, 3 hit, 4 defeat), z progress 0..1, w
//           facing (-1, 1)
//   bits.x  bit 0 elite, bits 1-3 variant, 8-15 seed

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec3 gIris;
vec3 gEyeInk;
float gTime;
float gSeed;

const int kBellcap = 0, kMochi = 1, kTurbanshell = 2, kCrab = 3, kGull = 4, kPudgy = 5, kStumpling = 6, kWisp = 7,
          kPebblet = 8, kFlitter = 9;

void eye(vec2 q, vec2 center, float size, int mood, float side)
{
    animeCritterEye(C, q, center, size, mood, side, gIris, gEyeInk, gTime, gSeed);
}

void cheeks(vec2 q, vec2 a, vec2 b, float size, vec3 ink)
{
    animeBlush(C, q, a, vec2(size, size * 0.5), 0.65, ink);
    animeBlush(C, q, b, vec2(size, size * 0.5), 0.65, ink);
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.w < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 p = vec2(local.x * facing, local.y) / k;
    int kind = int(s.params.x + 0.5);
    int state = int(s.params.y + 0.5);
    float progress = s.params.z;
    bool elite = (s.bits.x & 1u) != 0u;
    int variant = int((s.bits.x >> 1) & 7u);
    float seed = float((s.bits.x >> 8) & 0xffu) / 255.0;
    float time = s.time + seed * 10.0;
    vec3 bodyColor = s.color.rgb;
    vec3 accent = s.color2.rgb;
    vec2 feet = vec2(0.0, 50.0);
    bool flyer = kind == kWisp || kind == kGull || kind == kFlitter;

    C = animeBegin(p, 3.2);
    C.shadeDepth = 6.0;
    AnimeMotion m = animeMonsterMotion(state, progress, time, flyer ? 0.0 : 1.0);
    if (state == kAnimeMove && (kind == kPudgy || kind == kStumpling || kind == kTurbanshell || kind == kCrab || kind == kPebblet)) {
        float w = sin(progress * 2.0 * kAnimePi);
        m.offset.y = -abs(w) * (kind == kTurbanshell ? 0.0 : 2.6);
        m.squash = kind == kTurbanshell ? 1.0 + 0.06 * w : 1.0 + 0.04 * abs(w);
        m.tilt = kind == kStumpling ? 0.12 * w : kind == kCrab ? 0.0 : 0.04 * w;
        if (kind == kCrab) {
            m.offset.x = w * 2.0; // a sideways scuttle
        }
    }
    if (kind == kBellcap) {
        m.tilt *= 0.75;
    }
    if (flyer) {
        m.offset.y += -10.0 + sin(time * 2.4) * 4.0;
    }
    if (state == kAnimeMove && m.mood == kAnimeMoodCalm && !flyer) {
        m.mood = kAnimeMoodHappy * int(fract(seed * 3.0) > 0.5);
    }
    vec2 q = animeApplyMotion(p, m, feet);
    vec3 ink = animeInkOf(bodyColor * 0.6 + accent * 0.4);
    gIris = animeSaturate(mix(animeShadowTone(accent, 0.5), animeHex(0x5A3A9Au), 0.35), 1.6);
    if (kind == kPebblet || (kind == kMochi && variant == 1) || (kind == kTurbanshell && variant == 1)) {
        gIris = animeHex(0x2A9AC8u);
    }
    gEyeInk = animeInkOf(gIris * 0.3 + vec3(0.12, 0.05, 0.06));
    gTime = time;
    gSeed = seed;

    animeGroundShadow(C, p, feet, flyer ? 20.0 : (kind == kCrab || kind == kPudgy ? 34.0 : 29.0), m);
    if (elite) {
        animeAuraRing(C, p, feet, 40.0, time, mix(accent, vec3(1.0, 0.85, 0.3), 0.5));
    }

    float top = 0.0;
    float topX = 0.0;
    if (kind == kBellcap) {
        AnimeMaterial stalk = animeMaterial(bodyColor);
        AnimeMaterial cap = animeMaterial(accent);
        float stepPh = sin(progress * 2.0 * kAnimePi);
        animePart(C, sdEllipse(q - feet - vec2(-8.0, -3.5), vec2(7.0, 4.3)), animeTinted(stalk, vec3(0.85, 0.85, 0.92)));
        animePart(C, sdEllipse(q - feet - vec2(8.0, -3.5 - max(stepPh, 0.0) * 2.0), vec2(7.0, 4.3)), stalk);
        float body = sdRoundBox(q - vec2(0.0, 29.0), vec2(17.0, 18.0), 14.0);
        animePart(C, body, stalk);
        animeGloss(C, q, body, vec2(-8.0, 25.0), vec2(5.0, 6.0), 0.35);
        vec2 rim = vec2(0.0, 15.0 + sin(time * 3.4 + 0.6) * (state == kAnimeIdle ? 1.2 : 0.0));
        vec2 cq = rotate2d(q - rim, (state == kAnimeWindup ? 0.22 * smoothstep(0.0, 0.7, progress) : 0.0)) + rim;
        bool glowing = variant == 1;
        float capW = glowing ? 36.0 : 31.0;
        float capH = glowing ? 30.0 : 33.0;
        float capD = sdBellCap(cq, rim, capW, capH, glowing ? 0.0 : 5.0);
        animeCastShadow(C, body, sdBellCap(q - vec2(1.0, 4.0), rim, capW - 2.0, capH, 5.0), stalk);
        eye(q, vec2(-6.0, 29.0), 4.6, m.mood, -1.0);
        eye(q, vec2(9.0, 29.0), 4.6, m.mood, 1.0);
        cheeks(q, vec2(-12.0, 37.0), vec2(15.0, 37.0), 4.0, ink);
        animeMonsterMouth(C, q, vec2(1.5, 38.0), 6.5, m.mood, ink);
        if (glowing) {
            C.color = over(vec4(accent * 1.3, glowFalloff(capD, 10.0) * 0.5 * (0.8 + 0.2 * sin(time * 2.0))), C.color);
        }
        animePart(C, capD, cap);
        if (glowing) {
            // Glowing spots on a wide dome.
            float spots = max(animeSpots(cq * 1.0, 13.0, 3.4, 4.0), capD + 2.5);
            animePart(C, spots, animeGlow(mix(accent, vec3(1.0), 0.6)));
        } else {
            // A sprout on top: two little leaves.
            vec2 tip = rim + vec2(0.0, -capH - 2.0);
            animeStroke(C, sdBezier(cq, tip + vec2(0.0, 4.0), tip, tip + vec2(-1.0, -5.0)), 1.8, animeInkOf(animeHex(0x7CCB58u)));
            animePart(C, sdEar(cq, tip + vec2(-1.0, -4.0), tip + vec2(-9.0, -10.0 + sin(time * 3.0)), 3.6, 0.3), animeMaterial(animeHex(0x86D065u)));
            animePart(C, sdEar(cq, tip + vec2(-1.0, -4.0), tip + vec2(8.0, -12.0 - sin(time * 3.0)), 3.8, -0.3), animeMaterial(animeHex(0x86D065u)));
            // Cream dots on the cap.
            float dots = max(animeSpots(cq, 14.0, 3.0, 2.0), capD + 2.5);
            animePartNoLine(C, dots, animeMaterial(animeHex(0xFFF3D6u)));
        }
        animeGloss(C, cq, capD, rim + vec2(-12.0, -capH * 0.7), vec2(6.0, 7.0), 0.4);
        top = rim.y - capH - (glowing ? 4.0 : 14.0);
    } else if (kind == kMochi) {
        AnimeMaterial gel = animeMaterial(bodyColor);
        gel.shadeDepth = 1.3;
        bool ice = variant == 1;
        float body = sdEllipse(q - vec2(0.0, 33.0), vec2(30.0, 18.5));
        body = animeSmoothUnion(body, sdRoundBox(q - vec2(0.0, 44.0), vec2(29.0, 6.0), 6.0), 6.0);
        // The droplet's curl on top (a dewdrop), or ice crystals.
        if (!ice) {
            float sway = sin(time * 2.6) * 2.0;
            body = animeSmoothUnion(body, sdLock(q, vec2(-2.0, 18.0), vec2(2.0 + sway, 8.0), vec2(10.0 + sway, 6.0), 7.0, 1.2), 5.0);
        }
        body = max(body, q.y - 50.0);
        if (ice) {
            AnimeMaterial crystal = animeMaterial(mix(accent, animeHex(0xB8E8FFu), 0.5));
            for (int i = 0; i < 3; ++i) {
                float x = -12.0 + float(i) * 11.0;
                float hgt = 14.0 + float(i % 2) * 6.0;
                float d = sdTriangle(q, vec2(x - 5.0, 20.0), vec2(x + 5.0, 20.0), vec2(x + float(i - 1) * 2.0, 20.0 - hgt)) - 1.0;
                animePart(C, d, crystal);
                animeFlat(C, max(d + 1.5, abs(q.x - x - float(i - 1)) - 0.8), vec3(1.0));
            }
        }
        animePart(C, body, gel);
        float core = sdEllipse(q - vec2(-2.0, 35.0), vec2(20.0, 10.0));
        animeFlatAlpha(C, max(core, body + 3.0), mix(bodyColor, vec3(1.0), 0.45), 0.45);
        float bubble = sdCircle(q - vec2(-13.0 + sin(time * 1.3) * 3.0, 39.0 - fract(time * 0.25) * 6.0), 2.4);
        animeStroke(C, abs(bubble), 0.9, mix(bodyColor, vec3(1.0), 0.8));
        animeGloss(C, q, body, vec2(-15.0, 24.0), vec2(6.0, 4.5), 0.8);
        eye(q, vec2(-8.0, 31.0), 4.8, m.mood, -1.0);
        eye(q, vec2(10.0, 31.0), 4.8, m.mood, 1.0);
        cheeks(q, vec2(-16.0, 39.0), vec2(18.0, 39.0), 4.5, ink);
        animeMonsterMouth(C, q, vec2(1.0, 40.0), 6.5, m.mood, ink);
        top = ice ? 6.0 : 10.0;
    } else if (kind == kTurbanshell) {
        AnimeMaterial soft = animeMaterial(bodyColor);
        AnimeMaterial shell = animeMaterial(accent);
        bool rune = variant == 1;
        if (rune) {
            shell = animeMatte(accent);
        }
        float hide = state == kAnimeHit ? 1.0 - progress : (state == kAnimeDefeat ? 1.0 : 0.0);
        float stretch = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) * 3.0 : 0.0;
        vec2 head = vec2(23.0 + stretch - hide * 12.0, 32.0 + hide * 8.0);
        float body = sdRoundBox(q - vec2(2.0 + stretch * 0.5, 43.5), vec2(31.0 + stretch * 0.5, 6.5), 6.5);
        body = animeSmoothUnion(body, sdCircle(q - head, 10.5), 6.0);
        float sway = sin(time * 2.5) * 1.5;
        for (int i = 0; i < 2; ++i) {
            vec2 base = head + vec2(-2.0 + float(i) * 6.0, -6.0);
            vec2 tip = base + vec2(float(i) * 3.0 + sway, -9.0 + hide * 8.0);
            animePart(C, sdSegment(q, base, tip, 1.9), i == 0 ? animeTinted(soft, vec3(0.85, 0.85, 0.92)) : soft);
            animePart(C, sdCircle(q - tip, 2.3), soft);
        }
        animePart(C, body, soft);
        eye(q, head + vec2(-1.0, -1.0), 3.3, m.mood, -1.0);
        eye(q, head + vec2(6.5, -1.0), 3.0, m.mood, 1.0);
        animeMonsterMouth(C, q, head + vec2(3.5, 5.0), 4.5, m.mood, ink);
        // A round spiral shell.
        vec2 sc = vec2(-8.0, 24.0);
        float shellD = sdCircle(q - sc, 20.0);
        animePart(C, shellD, shell);
        animeStroke(C, max(animeSpiral(q, sc + vec2(1.0, 1.0), 17.0, 2.4), shellD + 1.0), 2.0, shell.ink);
        if (rune) {
            // Glowing rune marks in the stone.
            float runeD = max(abs(sdCircle(q - sc, 11.0)) - 1.2, abs(fract(atan(q.y - sc.y, q.x - sc.x) * 1.27 + time * 0.1) - 0.5) - 0.18);
            C.color = over(vec4(accent * 1.6 + vec3(0.2), animeCover(C, runeD) * (0.7 + 0.3 * sin(time * 3.0))), C.color);
        } else {
            float band = max(sdEllipse(q - sc - vec2(-4.0, 6.0), vec2(14.0, 3.5)), shellD + 2.0);
            animePartNoLine(C, band, animeMaterial(animeHex(0xFFF3D6u)));
        }
        animeGloss(C, q, shellD, sc + vec2(-8.0, -8.0), vec2(5.0, 4.0), 0.45);
        top = sc.y - 20.0;
        topX = sc.x;
    } else if (kind == kCrab) {
        // A round crab: a wide shell, two big claws held up, eye stalks, legs.
        AnimeMaterial shellM = animeMaterial(bodyColor);
        AnimeMaterial belly = animeMaterial(accent);
        float pinch = state == kAnimeWindup ? smoothstep(0.0, 0.7, progress) : (0.5 + 0.5 * sin(time * 4.0)) * 0.3;
        float walk = state == kAnimeMove ? sin(progress * 4.0 * kAnimePi) : 0.0;
        for (int i = 0; i < 3; ++i) {
            float x = -20.0 + float(i) * 8.0;
            animePart(C, sdLock(q, vec2(x, 40.0), vec2(x - 6.0, 44.0 + walk * float(i % 2) * 2.0), vec2(x - 9.0, 50.0), 2.4, 1.2),
                      animeTinted(shellM, vec3(0.8, 0.8, 0.9)));
            animePart(C, sdLock(q, vec2(-x, 40.0), vec2(-x + 6.0, 44.0 - walk * float(i % 2) * 2.0), vec2(-x + 9.0, 50.0), 2.4, 1.2),
                      shellM);
        }
        float body = sdEllipse(q - vec2(0.0, 33.0), vec2(28.0, 15.0));
        // Eye stalks behind the shell's top.
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -7.0 : 9.0;
            animePart(C, sdSegment(q, vec2(x, 24.0), vec2(x + 1.0, 14.0), 2.2), shellM);
        }
        animePart(C, body, shellM);
        animePartNoLine(C, max(body, -(q.y - 38.0)), belly);
        animeLine(C, max(abs(q.y - 38.0), body), shellM.ink);
        animeGloss(C, q, body, vec2(-10.0, 25.0), vec2(8.0, 4.0), 0.4);
        eye(q, vec2(-7.0, 12.0), 3.4, m.mood, -1.0);
        eye(q, vec2(10.0, 12.0), 3.4, m.mood, 1.0);
        animeMonsterMouth(C, q, vec2(2.0, 33.0), 6.0, m.mood, ink);
        cheeks(q, vec2(-12.0, 32.0), vec2(15.0, 32.0), 3.5, ink);
        // The claws, raised; they open and snap.
        for (int i = 0; i < 2; ++i) {
            float side = i == 0 ? -1.0 : 1.0;
            vec2 shoulder = vec2(side * 24.0, 30.0);
            vec2 hand = vec2(side * 32.0, 12.0 - pinch * 6.0);
            AnimeMaterial cm = i == 0 ? animeTinted(shellM, vec3(0.85, 0.85, 0.93)) : shellM;
            animePart(C, sdSegment(q, shoulder, hand, 3.8), cm);
            float open = 0.35 + 0.35 * pinch;
            float claw = sdEllipse(rotate2d(q - hand, side * 0.3), vec2(8.5, 7.5));
            float cut = sdTriangle(rotate2d(q - hand, side * 0.3), vec2(0.0, 0.0), vec2(side * 12.0, -8.0 - open * 8.0), vec2(side * 12.0, -1.0));
            animePart(C, max(claw, -cut), cm);
        }
        top = 6.0;
    } else if (kind == kGull) {
        // A puffy round seabird: flapping wings, an orange beak, tiny legs.
        AnimeMaterial down = animeMaterial(bodyColor);
        AnimeMaterial beakM = animeMaterial(accent);
        float flap = sin(time * 9.0 + (state == kAnimeWindup ? progress * 10.0 : 0.0));
        vec2 c0 = vec2(0.0, 28.0);
        float wingB = sdWing(q, c0 + vec2(-6.0, -4.0), 26.0, 0.6 + flap * 0.5, -1.0);
        animePart(C, wingB, animeTinted(down, vec3(0.82, 0.84, 0.94)));
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -4.0 : 5.0;
            animeStroke(C, sdSegment(q, vec2(x, 40.0), vec2(x, 46.0), 0.0), 1.6, beakM.ink);
        }
        float body = sdEllipse(q - c0, vec2(22.0, 18.0));
        body = animeSmoothUnion(body, sdLock(q, c0 + vec2(-18.0, 4.0), c0 + vec2(-26.0, 2.0), c0 + vec2(-32.0, -3.0), 6.0, 1.0), 4.0);
        animePart(C, body, down);
        animePartNoLine(C, max(body, sdEllipse(q - c0 - vec2(4.0, 8.0), vec2(14.0, 9.0))), animeMaterial(mix(bodyColor, vec3(1.0), 0.5)));
        float beak = sdTriangle(q, c0 + vec2(16.0, -4.0), c0 + vec2(16.0, 4.0), c0 + vec2(29.0, 1.0)) - 1.5;
        animePart(C, beak, beakM);
        eye(q, c0 + vec2(4.0, -5.0), 3.6, m.mood, -1.0);
        eye(q, c0 + vec2(13.0, -5.0), 3.2, m.mood, 1.0);
        cheeks(q, c0 + vec2(1.0, 3.0), c0 + vec2(15.0, 5.0), 3.0, ink);
        float wingF = sdWing(q, c0 + vec2(-2.0, 0.0), 24.0, 0.4 + flap * 0.6, -1.0);
        animePart(C, wingF, down);
        // A little cap tuft.
        animePart(C, sdEar(q, c0 + vec2(2.0, -16.0), c0 + vec2(-4.0, -24.0), 3.0, 0.4), down);
        top = c0.y - 20.0;
    } else if (kind == kPudgy) {
        AnimeMaterial hide = animeMaterial(bodyColor);
        AnimeMaterial inner = animeMaterial(accent);
        bool wool = variant == 1;
        float trot = sin(progress * 2.0 * kAnimePi) * (state == kAnimeMove ? 1.0 : 0.0);
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -16.0 : 10.0;
            animePart(C, sdRoundBox(q - vec2(x - trot * 3.0, 44.0), vec2(4.5, 6.5), 4.0), animeTinted(inner, vec3(0.82, 0.82, 0.92)));
        }
        animeStroke(C, animeSpiral(q, vec2(-31.0, 24.0), 4.5, 1.6), 2.2, hide.ink);
        float body = sdEllipse(q - vec2(0.0, 26.0), vec2(30.0, 22.5));
        if (wool) {
            // Fluffy fleece: puffs around the outline.
            for (int i = 0; i < 9; ++i) {
                float a = -2.8 + float(i) * 0.62;
                body = min(body, sdCircle(q - vec2(0.0, 26.0) - vec2(cos(a) * 26.0, sin(a) * 18.0), 8.0));
            }
        }
        animePart(C, body, hide);
        if (!wool) {
            // Barrel bands around the belly.
            for (int i = 0; i < 2; ++i) {
                float x = -10.0 + float(i) * 14.0;
                animePartNoLine(C, max(abs(q.x - x) - 2.4, body + 1.0), inner);
            }
        } else {
            for (int i = 0; i < 2; ++i) {
                float side = i == 0 ? -1.0 : 1.0;
                animePart(C, sdHorn(q, vec2(4.0 + side * 8.0, 6.0), vec2(4.0 + side * 16.0, -6.0), 3.6, side * 0.6),
                          animeMaterial(animeHex(0xF4E2C0u)));
            }
        }
        animeGloss(C, q, body, vec2(-12.0, 12.0), vec2(9.0, 5.0), 0.35);
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -10.0 : 16.0;
            animePart(C, sdRoundBox(q - vec2(x + trot * 3.0, 45.0), vec2(4.8, 6.0), 4.0), inner);
        }
        float earF = sdCircle(q - vec2(14.0, 3.0 + trot), 6.5);
        animePart(C, earF, inner);
        animeFlat(C, sdCircle(q - vec2(14.0, 4.0 + trot), 3.2), mix(accent, animeHex(0xF5B8C4u), 0.6));
        float snout = sdSnout(q, vec2(26.0, 30.0), vec2(7.5, 6.0));
        animePart(C, snout, animeMaterial(animeHex(0xF2B9A6u)));
        animeFlat(C, sdEllipse(q - vec2(24.0, 30.0), vec2(1.4, 2.2)), inner.ink);
        animeFlat(C, sdEllipse(q - vec2(28.5, 30.0), vec2(1.4, 2.2)), inner.ink);
        eye(q, vec2(10.0, 19.0), 4.4, m.mood, -1.0);
        eye(q, vec2(21.5, 19.0), 3.8, m.mood, 1.0);
        animeBlush(C, q, vec2(14.0, 30.0), vec2(4.0, 2.0), 0.7, ink);
        animeMonsterMouth(C, q, vec2(21.0, 39.5), 5.0, m.mood, ink);
        if (state == kAnimeMove) {
            for (int i = 0; i < 3; ++i) {
                float t = fract(progress * 2.0 + float(i) / 3.0);
                float d = sdCircle(q - vec2(-30.0 - t * 14.0, 47.0 - t * 6.0), 4.0 * (1.0 - t) + 1.0);
                C.color = over(vec4(vec3(0.95, 0.92, 0.85), animeCover(C, d) * (1.0 - t) * 0.8), C.color);
            }
        }
        top = 4.0;
        topX = 4.0;
    } else if (kind == kStumpling) {
        AnimeMaterial bark = animeMaterial(bodyColor);
        AnimeMaterial leaf = animeMaterial(accent);
        AnimeMaterial wood = animeMaterial(mix(bodyColor, animeHex(0xF1D9A8u), 0.65));
        float swing = state == kAnimeWindup ? -1.6 * smoothstep(0.0, 0.7, progress) : sin(time * 2.0) * 0.15;
        vec2 shoulderB = vec2(-17.0, 24.0);
        animePart(C, sdLock(q, shoulderB, shoulderB + vec2(-9.0, -4.0), shoulderB + rotate2d(vec2(-14.0, -10.0), -swing * 0.3), 4.0, 1.6),
                  animeTinted(bark, vec3(0.82, 0.82, 0.92)));
        for (int i = -1; i <= 1; ++i) {
            vec2 r = vec2(float(i) * 12.0, 44.0);
            float lift = state == kAnimeMove ? max(0.0, sin(progress * 2.0 * kAnimePi + float(i) * 2.0)) * 3.0 : 0.0;
            animePart(C, sdLock(q, r + vec2(0.0, -6.0), r + vec2(float(i) * 3.0, 0.0), r + vec2(float(i) * 6.0 + 2.0, 5.0 - lift), 5.0, 2.0), bark);
        }
        float logD = sdRoundBox(q - vec2(0.0, 22.0), vec2(20.0, 24.0), 9.0);
        animePart(C, logD, bark);
        animeLine(C, max(animeBark(q, 7.0, seed * 10.0), max(logD + 2.0, -(sdEllipse(q - vec2(3.0, 27.0), vec2(14.0, 11.0))))), bark.ink);
        vec2 topC = vec2(0.0, 0.5);
        float cut = sdEllipse(q - topC, vec2(19.0, 5.0));
        animePart(C, cut, wood);
        animeLine(C, max(abs(sdEllipse(q - topC, vec2(11.0, 3.0))), cut + 1.0), wood.ink);
        animeStroke(C, sdBezier(q, topC + vec2(4.0, -1.0), topC + vec2(4.0, -8.0), topC + vec2(2.0, -12.0)), 2.0, leaf.ink);
        animePart(C, sdEar(q, topC + vec2(2.5, -11.0), topC + vec2(-8.0, -18.0 + sin(time * 3.0)), 4.5, 0.25), leaf);
        animePart(C, sdEar(q, topC + vec2(3.0, -11.0), topC + vec2(13.0, -20.0 - sin(time * 3.0)), 5.0, -0.25), leaf);
        eye(q, vec2(-2.0, 24.0), 4.3, m.mood, -1.0);
        eye(q, vec2(11.0, 24.0), 4.3, m.mood, 1.0);
        animeMonsterMouth(C, q, vec2(5.0, 33.0), 6.0, m.mood, ink);
        cheeks(q, vec2(-7.0, 31.0), vec2(17.0, 31.0), 3.5, ink);
        vec2 shoulderF = vec2(18.0, 24.0);
        vec2 hand = shoulderF + rotate2d(vec2(12.0, 8.0), swing);
        animePart(C, sdLock(q, shoulderF, mix(shoulderF, hand, 0.5) + vec2(2.0, 3.0), hand, 4.2, 1.8), bark);
        animePart(C, sdEar(q, hand, hand + rotate2d(vec2(9.0, -2.0), swing), 3.5, 0.3), leaf);
        top = -6.0;
    } else if (kind == kWisp) {
        AnimeMaterial sheet = animeMaterial(bodyColor);
        float wave = sin(time * 3.0);
        bool leafy = variant == 1;
        float body = sdLockBelly(q, vec2(2.0, 6.0), vec2(4.0, 30.0), vec2(-26.0 + wave * 4.0, 44.0), 21.0, 2.0);
        body = animeSmoothUnion(body, sdCircle(q - vec2(2.0, 14.0), 19.0), 6.0);
        C.color = over(vec4(mix(bodyColor, vec3(1.0), 0.5), glowFalloff(body, 8.0) * 0.35), C.color);
        animePart(C, body, sheet);
        animeLine(C, max(sdBezier(q, vec2(-12.0, 30.0), vec2(-14.0, 37.0), vec2(-22.0 + wave * 3.0, 42.0)), body + 1.0), sheet.ink);
        animeGloss(C, q, body, vec2(-8.0, 4.0), vec2(6.0, 8.0), 0.5);
        eye(q, vec2(-4.0, 14.0), 4.8, m.mood, -1.0);
        eye(q, vec2(10.0, 14.0), 4.8, m.mood, 1.0);
        animeMonsterMouth(C, q, vec2(3.0, 25.0), 6.0, m.mood == kAnimeMoodCalm ? kAnimeMoodHappy : m.mood, ink);
        if (leafy) {
            AnimeMaterial leaf = animeMaterial(accent);
            animePart(C, sdEar(q, vec2(4.0, -4.0), vec2(-6.0, -16.0 + wave), 5.5, 0.3), leaf);
            animePart(C, sdEar(q, vec2(5.0, -4.0), vec2(14.0, -14.0 - wave), 5.0, -0.3), leaf);
            animeLine(C, max(sdSegment(q, vec2(4.0, -4.0), vec2(-4.0, -13.0 + wave), 0.0), sdEar(q, vec2(4.0, -4.0), vec2(-6.0, -16.0 + wave), 5.5, 0.3)),
                      leaf.ink);
        }
        C.alpha = 0.93;
        top = -6.0;
        topX = 2.0;
    } else if (kind == kPebblet) {
        // A round boulder buddy: mossy top, a crystal on its back, stubby feet.
        AnimeMaterial stone = animeMatte(bodyColor);
        AnimeMaterial crystal = animeMaterial(accent);
        float stepPh = sin(progress * 2.0 * kAnimePi) * (state == kAnimeMove ? 1.0 : 0.0);
        float crys = sdTriangle(q, vec2(-22.0, 18.0), vec2(-10.0, 12.0), vec2(-22.0, -4.0)) - 1.5;
        crys = min(crys, sdTriangle(q, vec2(-14.0, 14.0), vec2(-4.0, 10.0), vec2(-10.0, -2.0)) - 1.2);
        C.color = over(vec4(accent * 1.3, glowFalloff(crys, 7.0) * 0.4), C.color);
        animePart(C, crys, crystal);
        animeFlat(C, max(crys + 1.5, abs(q.x + 18.0 - (q.y - 6.0) * 0.1) - 0.8), vec3(1.0));
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -9.0 : 10.0;
            animePart(C, sdRoundBox(q - vec2(x + (i == 0 ? -stepPh : stepPh) * 3.0, 46.0), vec2(6.0, 4.5), 3.5),
                      i == 0 ? animeTinted(stone, vec3(0.85, 0.85, 0.92)) : stone);
        }
        float body = sdEllipse(q - vec2(0.0, 29.0), vec2(26.0, 19.0));
        body = animeSmoothUnion(body, sdCircle(q - vec2(-8.0, 17.0), 12.0), 6.0);
        animePart(C, body, stone);
        float moss = max(body, q.y - 16.0 - 3.0 * sin(q.x * 0.5));
        animePartNoLine(C, moss, animeMaterial(animeHex(0x7CC060u)));
        animeLine(C, max(abs(q.y - 16.0 - 3.0 * sin(q.x * 0.5)), body + 0.8), animeInkOf(animeHex(0x7CC060u)));
        animeLine(C, max(sdBezier(q, vec2(14.0, 36.0), vec2(18.0, 40.0), vec2(16.0, 44.0)), body + 1.0), stone.ink);
        eye(q, vec2(2.0, 28.0), 4.4, m.mood, -1.0);
        eye(q, vec2(15.0, 28.0), 4.0, m.mood, 1.0);
        cheeks(q, vec2(-3.0, 36.0), vec2(20.0, 36.0), 3.5, ink);
        animeMonsterMouth(C, q, vec2(9.0, 37.0), 5.5, m.mood, ink);
        top = 6.0;
        topX = -6.0;
    } else {
        // Flitter: a round fuzzy bat with big ears and flapping wings.
        AnimeMaterial fur = animeMaterial(bodyColor);
        AnimeMaterial wingM = animeMaterial(accent);
        float flap = sin(time * 12.0);
        vec2 c0 = vec2(0.0, 26.0);
        animePart(C, sdWing(q, c0 + vec2(-8.0, -2.0), 30.0, 0.5 + flap * 0.6, -1.0), animeTinted(wingM, vec3(0.8, 0.82, 0.94)));
        float body = sdCircle(q - c0, 17.0);
        for (int i = 0; i < 2; ++i) {
            float side = i == 0 ? -1.0 : 1.0;
            body = animeSmoothUnion(body, sdEar(q, c0 + vec2(side * 7.0, -12.0), c0 + vec2(side * 13.0, -28.0), 6.0, side * 0.25), 3.0);
        }
        animePart(C, body, fur);
        animePartNoLine(C, max(body, sdEllipse(q - c0 - vec2(2.0, 8.0), vec2(10.0, 7.0))), animeMaterial(mix(bodyColor, vec3(1.0), 0.4)));
        for (int i = 0; i < 2; ++i) {
            float side = i == 0 ? -1.0 : 1.0;
            animeFlat(C, sdEar(q, c0 + vec2(side * 8.0, -14.0), c0 + vec2(side * 12.0, -24.0), 3.0, side * 0.25), mix(accent, vec3(1.0), 0.3));
        }
        eye(q, c0 + vec2(-4.0, -1.0), 3.8, m.mood, -1.0);
        eye(q, c0 + vec2(7.0, -1.0), 3.6, m.mood, 1.0);
        animeMonsterMouth(C, q, c0 + vec2(2.0, 8.0), 5.0, m.mood == kAnimeMoodCalm ? kAnimeMoodHappy : m.mood, ink);
        animeFlat(C, sdTriangle(q, c0 + vec2(-1.0, 9.0), c0 + vec2(2.0, 9.0), c0 + vec2(0.5, 12.0)), vec3(1.0)); // a fang
        animePart(C, sdWing(q, c0 + vec2(4.0, 2.0), 28.0, 0.4 + flap * 0.7, 1.0), wingM);
        top = c0.y - 30.0;
    }

    if (elite) {
        animeCrown(C, q, vec2(topX, top + 3.0), 20.0, -0.12);
    }
    AnimeCanvas pop = C;
    pop.color = animeMotionFinish(C, p, m, accent);
    animePop(pop, p, feet - vec2(0.0, 20.0), m.state == kAnimeDefeat ? m.progress : 0.0, mix(bodyColor, accent, 0.5), 40.0);
    outColor = pop.color;
    outColor.a *= s.color.a;
}
