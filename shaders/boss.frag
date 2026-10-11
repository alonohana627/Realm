#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_monster.glsl"

// Realm's three area bosses, big multi-part versions of their region's
// creatures with the cast's anime eyes and ink lines:
//   0 Thornbark - the old tree of the grove: a huge stump body with a face
//     in the bark, a crown of leaves and mushrooms, two branch arms and root
//     feet;
//   1 Frostmane - the beast of the summit: a giant fluffy snow hog with
//     curled horns, an icy mane, a snout and stomping hooves;
//   2 Murkmaw - the gloom under the harbor: a floating dark jelly with a
//     crown of crystals, wisp arms and a glowing heart inside.
// Phases: 0 calm colors, 1 angry (narrowed eyes, lowered brows), 2 enraged
// (a glowing aura and glowing markings).
//
// Design space: 128 units over the quad height, feet on y = +50, facing +x.
//   color   body          color2  accent (leaves, mane, gloom glow)
//   params  x phase, y state (0 idle, 1 move, 2 wind-up, 3 hit, 4 defeat),
//           z progress, w facing
//   bits.x  0-2 kind

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec3 gIris;
vec3 gEyeInk;
float gTime;

void eye(vec2 q, vec2 center, float size, int mood, float side, int phase)
{
    animeCritterEye(C, q, center, size, mood, side, gIris, gEyeInk, gTime, 0.3);
    if (phase >= 1 && mood != kAnimeMoodKnocked && mood != kAnimeMoodHurt) {
        // Angry: inner brows down, a heavy lid over the top of the eye.
        float h = size * 3.0;
        animeBrow(C, q, center + vec2(0.0, -h * 0.72), h * 0.95, side, 1.0, gEyeInk);
    }
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.w < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 p = vec2(local.x * facing, local.y) / k;
    int phase = int(s.params.x + 0.5);
    int state = int(s.params.y + 0.5);
    float progress = s.params.z;
    int kind = int(s.bits.x & 7u);
    float time = s.time;
    vec3 body = s.color.rgb;
    vec3 accent = s.color2.rgb;
    vec2 feet = vec2(0.0, 50.0);
    gTime = time;

    C = animeBegin(p, 2.2);
    C.shadeDepth = 4.5;
    AnimeMotion m = animeMonsterMotion(state, progress, time, 0.45);
    if (kind == 2) {
        m.offset.y += -8.0 + sin(time * 1.8) * 3.0;
    }
    int mood = m.mood;
    if (state == kAnimeIdle || state == kAnimeMove) {
        mood = phase >= 1 ? kAnimeMoodAngry : kAnimeMoodCalm;
    }
    vec2 q = animeApplyMotion(p, m, feet);
    vec3 ink = animeInkOf(body * 0.6 + accent * 0.4);
    gIris = phase >= 1 ? animeHex(0xE04040u) : animeSaturate(mix(animeShadowTone(accent, 0.5), animeHex(0x5A3A9Au), 0.35), 1.6);
    gEyeInk = animeInkOf(gIris * 0.3 + vec3(0.12, 0.05, 0.06));

    animeGroundShadow(C, p, feet, 40.0, m);
    if (phase >= 2) {
        animeAuraRing(C, p, feet, 52.0, time * 1.5, mix(accent, vec3(1.0, 0.4, 0.3), 0.4));
    }

    if (kind == 0) {
        // ---- Thornbark.
        AnimeMaterial bark = animeMaterial(body);
        AnimeMaterial leaf = animeMaterial(accent);
        AnimeMaterial wood = animeMaterial(mix(body, animeHex(0xF1D9A8u), 0.6));
        float swing = state == kAnimeWindup ? -1.4 * smoothstep(0.0, 0.7, progress) : sin(time * 1.4) * 0.12;
        // The back arm.
        vec2 sb = vec2(-24.0, 6.0);
        animePart(C, sdLock(q, sb, sb + vec2(-14.0, -8.0), sb + rotate2d(vec2(-24.0, -18.0), -swing * 0.4), 6.0, 2.0), animeTinted(bark, vec3(0.8, 0.8, 0.9)));
        animePart(C, sdEar(q, sb + rotate2d(vec2(-24.0, -18.0), -swing * 0.4), sb + rotate2d(vec2(-34.0, -30.0), -swing * 0.4), 7.0, 0.3),
                  animeTinted(leaf, vec3(0.8, 0.85, 0.9)));
        // Root feet.
        for (int i = -1; i <= 1; ++i) {
            vec2 r = vec2(float(i) * 16.0, 44.0);
            float lift = state == kAnimeMove ? max(0.0, sin(progress * 2.0 * kAnimePi + float(i) * 2.0)) * 3.0 : 0.0;
            animePart(C, sdLock(q, r + vec2(0.0, -8.0), r + vec2(float(i) * 4.0, 0.0), r + vec2(float(i) * 9.0 + 2.0, 6.0 - lift), 7.0, 2.5), bark);
        }
        // The great stump.
        float trunk = sdRoundBox(q - vec2(0.0, 14.0), vec2(28.0, 32.0), 12.0);
        animePart(C, trunk, bark);
        animeLine(C, max(animeBark(q, 8.0, 3.0), max(trunk + 3.0, -(sdEllipse(q - vec2(4.0, 12.0), vec2(20.0, 16.0))))), bark.ink);
        // A mossy patch.
        animePartNoLine(C, max(trunk + 1.0, sdEllipse(q - vec2(-18.0, 30.0), vec2(12.0, 8.0))), animeMaterial(animeHex(0x7CC060u)));
        // The cut top with rings, a crown of leaves and two mushrooms.
        vec2 top = vec2(0.0, -16.0);
        animePart(C, sdEllipse(q - top, vec2(27.0, 6.0)), wood);
        animeLine(C, max(abs(sdEllipse(q - top, vec2(16.0, 3.5))), sdEllipse(q - top, vec2(27.0, 6.0)) + 1.0), wood.ink);
        for (int i = 0; i < 5; ++i) {
            float fi = float(i);
            vec2 base = top + vec2(-18.0 + fi * 9.0, -2.0);
            vec2 tip = base + vec2((fi - 2.0) * 5.0 + sin(time * 2.0 + fi) * 2.0, -20.0 - abs(fi - 2.0) * -4.0);
            animePart(C, sdEar(q, base, tip, 6.5, (fi - 2.0) * 0.15), leaf);
        }
        animePart(C, sdBellCap(q, vec2(20.0, -18.0), 9.0, 9.0, 2.0), animeMaterial(animeHex(0xEC5662u)));
        animePart(C, sdBellCap(q, vec2(-22.0, -14.0), 7.0, 7.0, 2.0), animeMaterial(animeHex(0xF4C24Eu)));
        // The face.
        eye(q, vec2(-6.0, 6.0), 5.5, mood, -1.0, phase);
        eye(q, vec2(12.0, 6.0), 5.5, mood, 1.0, phase);
        animeMonsterMouth(C, q, vec2(4.0, 22.0), 12.0, mood, ink);
        if (phase >= 2) {
            float glowLines = max(animeBark(q, 8.0, 3.0), trunk + 4.0);
            C.color = over(vec4(animeHex(0xFF8A3Au) * 1.5, animeCover(C, glowLines - 0.8) * (0.6 + 0.3 * sin(time * 6.0))), C.color);
        }
        // The front arm, swinging on the wind-up.
        vec2 sf = vec2(26.0, 6.0);
        vec2 hand = sf + rotate2d(vec2(18.0, 10.0), swing);
        animePart(C, sdLock(q, sf, mix(sf, hand, 0.5) + vec2(3.0, 4.0), hand, 6.0, 2.5), bark);
        animePart(C, sdEar(q, hand, hand + rotate2d(vec2(14.0, -4.0), swing), 6.0, 0.3), leaf);
    } else if (kind == 1) {
        // ---- Frostmane.
        AnimeMaterial fleece = animeMatte(body);
        AnimeMaterial mane = animeMaterial(accent);
        AnimeMaterial horn = animeMaterial(animeHex(0xF2E6C8u));
        float trot = state == kAnimeMove ? sin(progress * 2.0 * kAnimePi) : 0.0;
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -22.0 : 12.0;
            animePart(C, sdRoundBox(q - vec2(x - trot * 4.0, 42.0), vec2(8.0, 9.0), 5.0), animeTinted(animeMaterial(animeHex(0x6A7A9Au)), vec3(0.85)));
        }
        float bodyD = sdEllipse(q - vec2(-4.0, 16.0), vec2(40.0, 30.0));
        for (int i = 0; i < 11; ++i) {
            float ang = -3.0 + float(i) * 0.55;
            bodyD = min(bodyD, sdCircle(q - vec2(-4.0, 16.0) - vec2(cos(ang) * 36.0, sin(ang) * 26.0), 9.5));
        }
        animePart(C, bodyD, fleece);
        animeGloss(C, q, bodyD, vec2(-20.0, -2.0), vec2(12.0, 7.0), 0.4);
        for (int i = 0; i < 2; ++i) {
            float x = i == 0 ? -12.0 : 22.0;
            animePart(C, sdRoundBox(q - vec2(x + trot * 4.0, 43.0), vec2(8.0, 8.0), 5.0), animeMaterial(animeHex(0x6A7A9Au)));
        }
        // The icy mane over the head.
        float maneD = 1e5;
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            vec2 base = vec2(6.0 + fi * 5.0, -8.0 + fi * 2.0);
            maneD = min(maneD, sdLock(q, base, base + vec2(-8.0, -6.0), base + vec2(-18.0 + fi * 2.0, -2.0 + fi * 3.0), 6.0, 1.0));
        }
        animePart(C, maneD, mane);
        // The head: a round snout and curled horns.
        vec2 head = vec2(24.0, 10.0);
        float headD = sdCircle(q - head, 17.0);
        animePart(C, headD, fleece);
        for (int i = 0; i < 2; ++i) {
            float side = i == 0 ? -1.0 : 1.0;
            animePart(C, sdHorn(q, head + vec2(side * 9.0 - 2.0, -12.0), head + vec2(side * 18.0 - 4.0, -26.0), 5.0, side * 0.8),
                      i == 0 ? animeTinted(horn, vec3(0.85)) : horn);
        }
        float snout = sdSnout(q, head + vec2(14.0, 8.0), vec2(9.0, 7.0));
        animePart(C, snout, animeMaterial(animeHex(0xC8D8F0u)));
        animeFlat(C, sdEllipse(q - head - vec2(12.0, 8.0), vec2(1.8, 2.6)), ink);
        animeFlat(C, sdEllipse(q - head - vec2(18.0, 8.0), vec2(1.8, 2.6)), ink);
        eye(q, head + vec2(-2.0, -2.0), 4.4, mood, -1.0, phase);
        eye(q, head + vec2(9.0, -2.0), 4.0, mood, 1.0, phase);
        animeMonsterMouth(C, q, head + vec2(10.0, 15.0), 7.0, mood, ink);
        if (phase >= 2) {
            C.color = over(vec4(accent * 1.6, glowFalloff(maneD, 6.0) * (0.5 + 0.3 * sin(time * 5.0))), C.color);
        }
    } else {
        // ---- Murkmaw.
        AnimeMaterial gloom = animeMaterial(body);
        gloom.shadeDepth = 1.4;
        float wave = sin(time * 2.0);
        // Wisp arms trailing behind.
        for (int i = 0; i < 3; ++i) {
            float fi = float(i);
            vec2 base = vec2(-14.0 + fi * 14.0, 30.0);
            vec2 tip = base + vec2(-10.0 + sin(time * 2.0 + fi) * 6.0, 20.0);
            animePart(C, sdLock(q, base, base + vec2(-4.0, 10.0), tip, 7.0, 1.0), animeTinted(gloom, vec3(0.8, 0.8, 0.95)));
        }
        float bodyD = sdEllipse(q - vec2(0.0, 10.0), vec2(38.0, 30.0 + wave * 1.5));
        bodyD = animeSmoothUnion(bodyD, sdRoundBox(q - vec2(0.0, 30.0), vec2(34.0, 8.0), 8.0), 8.0);
        C.color = over(vec4(accent * 1.2, glowFalloff(bodyD, 14.0) * 0.4), C.color);
        animePart(C, bodyD, gloom);
        // The glowing heart inside.
        float heart = sdCircle(q - vec2(-6.0, 18.0), 9.0 + sin(time * 3.0) * 1.0);
        animeFlatAlpha(C, max(heart, bodyD + 2.0), accent * 1.5, 0.8);
        animeGloss(C, q, bodyD, vec2(-18.0, -6.0), vec2(10.0, 7.0), 0.5);
        // The crown of crystals.
        for (int i = 0; i < 5; ++i) {
            float fi = float(i);
            float x = -20.0 + fi * 10.0;
            float hgt = 12.0 + (i == 2 ? 10.0 : (i % 2) * 4.0);
            float d = sdTriangle(q, vec2(x - 5.0, -16.0), vec2(x + 5.0, -16.0), vec2(x, -16.0 - hgt)) - 1.0;
            C.color = over(vec4(accent * 1.4, glowFalloff(d, 6.0) * 0.5), C.color);
            animePart(C, d, animeMaterial(mix(accent, vec3(1.0), 0.3)));
        }
        eye(q, vec2(-4.0, 4.0), 6.0, mood, -1.0, phase);
        eye(q, vec2(16.0, 4.0), 6.0, mood, 1.0, phase);
        animeMonsterMouth(C, q, vec2(6.0, 22.0), 12.0, mood, ink);
        C.alpha = 0.95;
    }

    AnimeCanvas pop = C;
    pop.color = animeMotionFinish(C, p, m, accent);
    animePop(pop, p, feet - vec2(0.0, 30.0), m.state == kAnimeDefeat ? m.progress : 0.0, mix(body, accent, 0.5), 60.0);
    outColor = pop.color;
    outColor.a *= s.color.a;
}
