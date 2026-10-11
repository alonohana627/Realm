#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_ui.glsl"
#include "gamelib/anime_building.glsl"

// Realm's interface: GameLib's soft anime widgets (windows, wells, tooltips,
// pill buttons, item slots, gauges, key caps, arrows, quest markers, damage
// digits) plus Realm's own pieces - tabs, the level badge, name plates, the
// clockwise cooldown sweep, the world map's painted sea and lands - and
// every icon: potions, scrolls, monster drops, quest items, equipment by
// slot and style, the nineteen skills and the stats.
//
// Panels fill their quad less kAnimeUiMargin (the game grows the quad);
// icons are drawn in 128 design units over the quad's height.
//   color / color2  main and accent colors
//   params  x kind (app Ui); panels: window y title height, z style, w
//           radius; button y state; slot y rarity glow, z selected; gauge y
//           fill, z trail; tab y selected; sweep y the remaining cooldown
//           0..1; icons: y family / id, z style, w extra

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const int kWindow = 0, kButton = 1, kSlot = 2, kGauge = 3, kItemIcon = 4, kSkillIcon = 5, kStatIcon = 6, kMarker = 7, kDigit = 8,
          kCoin = 9, kKeyCap = 10, kWell = 11, kArrow = 12, kTooltip = 13, kSweep = 14, kTab = 15, kBadge = 16, kPlate = 17,
          kLogo = 18, kMap = 19, kShadow = 20;

AnimeCanvas C;

// ---------------------------------------------------------------------------
// Icons (128 design units, centered)

void potion(vec2 p, vec3 liquid, float size)
{
    AnimeMaterial glass = animeMaterial(mix(liquid, vec3(1.0), 0.15));
    float body = sdCircle(p - vec2(0.0, 12.0), 30.0 * size);
    float neck = sdRoundBox(p - vec2(0.0, -24.0 * size), vec2(11.0, 12.0) * size, 3.0);
    float bottle = animeSmoothUnion(body, neck, 6.0);
    animePart(C, bottle, glass);
    animeFlat(C, max(bottle + 3.0, -(p.y - 6.0 * size)), mix(liquid, vec3(1.0), 0.2));
    animePart(C, sdRoundBox(p - vec2(0.0, -40.0 * size), vec2(13.0, 7.0) * size, 3.0), animeMaterial(animeHex(0xB07A45u)));
    animeGloss(C, p, bottle, vec2(-12.0, 0.0) * size, vec2(7.0, 11.0) * size, 0.8);
}

void itemIcon(vec2 p, int family, int style, vec3 a, vec3 b, float time)
{
    if (family >= 1 && family <= 6) {
        vec3 liquid = family == 1 ? animeHex(0xF25F6Eu) : family == 2 ? animeHex(0xF89A48u) : family == 3 ? animeHex(0xF6F2F0u)
                    : family == 4 ? animeHex(0x5E9CF2u) : family == 5 ? animeHex(0xA06CE4u) : animeHex(0xF4C24Eu);
        potion(p, liquid, family == 6 ? 0.95 : 0.9);
        if (family == 6) {
            animePart(C, sdStar(p - vec2(18.0, -22.0), 5, 10.0, 4.5) - 1.0, animeGlow(animeHex(0xFFF2B0u)));
        }
    } else if (family == 7) {
        // A rolled scroll with a ribbon.
        AnimeMaterial paper = animeMatte(animeHex(0xF4E6C4u));
        float roll = sdRoundBox(p, vec2(36.0, 24.0), 6.0);
        animePart(C, roll, paper);
        for (int i = -1; i <= 1; i += 2) {
            animePart(C, sdRoundBox(p - vec2(float(i) * 38.0, 0.0), vec2(7.0, 28.0), 6.0), animeTinted(paper, vec3(0.9)));
        }
        animePart(C, sdRoundBox(p, vec2(5.0, 26.0), 2.0), animeMaterial(animeHex(0xEC5662u)));
        animeLine(C, max(abs(p.y + 8.0) - 0.0, roll + 6.0), paper.ink);
        animeLine(C, max(abs(p.y - 8.0) - 0.0, roll + 6.0), paper.ink);
    } else if (family >= 20 && family < 40) {
        // Monster drops, one each.
        int k = family - 20;
        if (k == 0) {
            animePart(C, sdBellCap(p - vec2(0.0, 0.0), vec2(0.0, 18.0), 38.0, 38.0, 6.0), animeMaterial(animeHex(0xF2788Au)));
            animePart(C, sdRoundBox(p - vec2(0.0, 26.0), vec2(14.0, 14.0), 10.0), animeMaterial(animeHex(0xFFEEDAu)));
        } else if (k == 1) {
            float d = sdLock(p, vec2(0.0, -40.0), vec2(-6.0, 0.0), vec2(0.0, 30.0), 2.0, 26.0);
            d = min(d, sdCircle(p - vec2(0.0, 18.0), 26.0));
            animePart(C, d, animeMaterial(animeHex(0x8EDCF6u)));
            animeGloss(C, p, d, vec2(-10.0, 10.0), vec2(6.0, 9.0), 0.8);
        } else if (k == 2 || k == 13) {
            float d = sdCircle(p, 36.0);
            animePart(C, d, animeMaterial(k == 2 ? animeHex(0xF0A050u) : animeHex(0x9C9AB8u)));
            animeStroke(C, max(animeSpiral(p, vec2(2.0, 2.0), 30.0, 2.4), d + 1.0), 4.0, animeInkOf(animeHex(0xF0A050u)));
            if (k == 13) {
                C.color = over(vec4(animeHex(0x8CF0E0u) * 1.4, animeCover(C, abs(sdCircle(p, 18.0)) - 3.0)), C.color);
            }
        } else if (k == 3) {
            float claw = sdEllipse(p - vec2(-4.0, 6.0), vec2(34.0, 28.0));
            float cut = sdTriangle(p, vec2(0.0, 6.0), vec2(44.0, -24.0), vec2(44.0, 10.0));
            animePart(C, max(claw, -cut), animeMaterial(animeHex(0xF2705Au)));
        } else if (k == 4) {
            animePart(C, sdLock(p, vec2(-30.0, 30.0), vec2(0.0, -10.0), vec2(30.0, -36.0), 4.0, 16.0), animeMaterial(animeHex(0xF8F8FFu)));
            animeLine(C, sdBezier(p, vec2(-28.0, 28.0), vec2(0.0, -6.0), vec2(26.0, -30.0)), animeInkOf(animeHex(0xF8F8FFu)));
        } else if (k == 5) {
            for (int i = 0; i < 5; ++i) {
                float x = -20.0 + float(i) * 10.0;
                animePart(C, sdLock(p, vec2(x, 30.0), vec2(x + 2.0, 0.0), vec2(x - 4.0 + float(i), -30.0), 4.0, 1.0), animeMaterial(animeHex(0xA8703Eu)));
            }
        } else if (k == 6) {
            animePart(C, sdLock(p, vec2(-30.0, 30.0), vec2(0.0, 0.0), vec2(30.0, -24.0), 7.0, 4.0), animeMaterial(animeHex(0xB07A48u)));
            animePart(C, sdEar(p, vec2(20.0, -16.0), vec2(36.0, -40.0), 9.0, 0.3), animeMaterial(animeHex(0x7CCB58u)));
        } else if (k == 7) {
            float d = sdCircle(p, 32.0);
            C.color = over(vec4(animeHex(0x5AD8E8u) * 1.3, glowFalloff(d, 16.0) * 0.6), C.color);
            animePart(C, d, animeGlow(animeHex(0x9AF0F8u)));
        } else if (k == 8) {
            animePart(C, sdEar(p, vec2(-28.0, 30.0), vec2(28.0, -34.0), 22.0, 0.3), animeMaterial(animeHex(0x8CD86Au)));
            animeLine(C, sdSegment(p, vec2(-24.0, 26.0), vec2(22.0, -28.0), 0.0), animeInkOf(animeHex(0x8CD86Au)));
        } else if (k == 9) {
            float d = 1e5;
            for (int i = 0; i < 5; ++i) {
                float a = float(i) * 1.2566;
                d = min(d, sdCircle(p - vec2(cos(a), sin(a)) * 18.0, 18.0));
            }
            animePart(C, d, animeMatte(animeHex(0xF6F8FFu)));
        } else if (k == 10) {
            float d = sdEllipse(p, vec2(34.0, 30.0));
            animePart(C, d, animeMatte(animeHex(0xB8B0C8u)));
            animePart(C, sdCircle(p - vec2(4.0, 2.0), 12.0), animeGlow(animeHex(0xFF9A6Au)));
        } else if (k == 11) {
            float d = sdCircle(p, 28.0);
            animePart(C, d, animeMaterial(animeHex(0xBCE8FFu)));
            animeGloss(C, p, d, vec2(-10.0, -10.0), vec2(8.0, 8.0), 0.9);
        } else if (k == 12) {
            float d = sdWing(p, vec2(-24.0, 0.0), 54.0, 0.6, 1.0);
            animePart(C, d, animeMaterial(animeHex(0x8A72C8u)));
        }
    } else if (family >= 40 && family < 60) {
        int k = family - 40;
        if (k == 0 || k == 1) {
            // A letter or a parcel.
            AnimeMaterial paper = animeMatte(k == 0 ? animeHex(0xFFF6E4u) : animeHex(0xD8B080u));
            float d = sdRoundBox(p, vec2(40.0, 28.0), 4.0);
            animePart(C, d, paper);
            if (k == 0) {
                animeLine(C, max(min(sdSegment(p, vec2(-40.0, -28.0), vec2(0.0, 4.0), 0.0), sdSegment(p, vec2(40.0, -28.0), vec2(0.0, 4.0), 0.0)), d + 1.0), paper.ink);
                animePart(C, sdCircle(p - vec2(0.0, 4.0), 8.0), animeMaterial(animeHex(0xEC5662u)));
            } else {
                animePart(C, max(abs(p.x) - 4.0, d + 0.5), animeMaterial(animeHex(0xEC5662u)));
                animePart(C, max(abs(p.y) - 4.0, d + 0.5), animeMaterial(animeHex(0xEC5662u)));
            }
        } else if (k == 5) {
            float ring = abs(sdCircle(p, 32.0)) - 6.0;
            animePart(C, ring, animeMetal(animeHex(0xC8A060u)));
            animeFlat(C, sdCircle(p, 26.0), mix(animeHex(0xBFE8FFu), vec3(1.0), 0.3));
            animeLine(C, max(sdSegment(p, vec2(-10.0, -20.0), vec2(14.0, 18.0), 0.0), sdCircle(p, 26.0)), animeHex(0x6A8AA8u));
        } else {
            // The beacon's shards.
            vec3 col = k == 2 ? animeHex(0xFFC84Au) : k == 3 ? animeHex(0x8CC8FFu) : animeHex(0xF07AD0u);
            float d = sdTriangle(p, vec2(-24.0, 30.0), vec2(26.0, 24.0), vec2(4.0, -44.0)) - 3.0;
            C.color = over(vec4(col * 1.4, glowFalloff(d, 18.0) * (0.5 + 0.2 * sin(time * 3.0))), C.color);
            animePart(C, d, animeGlow(mix(col, vec3(1.0), 0.3)));
            animeFlat(C, max(d + 4.0, abs(p.x + 2.0 + p.y * 0.2) - 2.5), vec3(1.0));
        }
    } else if (family >= 60 && family < 70) {
        // Equipment by slot and style, in its colors.
        int slot = family - 60;
        AnimeMaterial main = animeMaterial(a);
        AnimeMaterial steel = animeMetal(animeHex(0xD2DCEEu));
        AnimeMaterial wood = animeMaterial(animeHex(0xA8703Eu));
        if (slot == 0) {
            if (style == 2 || style == 6) {
                float len = style == 6 ? 46.0 : 40.0;
                vec2 g = vec2(-24.0, 28.0);
                vec2 dir = normalize(vec2(1.0, -1.0));
                vec2 tip = g + dir * len * 1.4;
                animePartShine(C, sdLock(p, g + dir * 10.0, g + dir * 40.0, tip, style == 6 ? 9.0 : 7.0, 2.0), steel,
                               animeCover(C, abs(sdSegment(p, g + dir * 10.0, tip, 0.0) + 2.0) - 1.2) * 0.8, vec3(1.0));
                animePart(C, sdSegment(p, g + dir * 10.0 + vec2(-10.0, -10.0), g + dir * 10.0 + vec2(10.0, 10.0), 4.0), animeMetal(b));
                animePart(C, sdSegment(p, g - dir * 6.0, g + dir * 8.0, 4.0), animeMaterial(animeHex(0x7A4A2Eu)));
            } else if (style == 3) {
                animePart(C, sdSegment(p, vec2(-26.0, 36.0), vec2(20.0, -30.0), 4.5), wood);
                animePart(C, sdLockBelly(p, vec2(12.0, -24.0), vec2(30.0, -20.0), vec2(36.0, 0.0), 8.0, 14.0), steel);
            } else if (style == 4 || style == 5) {
                bool staff = style == 4;
                animePart(C, sdSegment(p, vec2(-24.0, 40.0), vec2(14.0, staff ? -26.0 : -14.0), staff ? 4.5 : 4.0), wood);
                vec2 top = staff ? vec2(18.0, -32.0) : vec2(18.0, -22.0);
                float gem = staff ? sdCircle(p - top, 12.0) : sdStar(p - top, 5, 16.0, 7.0) - 1.0;
                C.color = over(vec4(b * 1.3, glowFalloff(gem, 12.0) * 0.6), C.color);
                animePart(C, gem, animeGlow(mix(b, vec3(1.0), 0.35)));
            } else {
                animePart(C, sdLock(p, vec2(-26.0, 34.0), vec2(0.0, 0.0), vec2(26.0, -34.0), 7.0, 9.0), wood);
                animePart(C, sdEar(p, vec2(8.0, -8.0), vec2(26.0, -4.0), 7.0, 0.3), animeMaterial(animeHex(0x7CCB58u)));
            }
        } else if (slot == 1) {
            // Hats.
            if (style == 2) {
                animePart(C, sdEllipse(p - vec2(0.0, 24.0), vec2(48.0, 10.0)), main);
                animePart(C, sdLock(p, vec2(0.0, 20.0), vec2(4.0, -16.0), vec2(-18.0, -42.0), 22.0, 3.0), main);
                animePart(C, sdStar(p - vec2(-18.0, -42.0), 5, 8.0, 3.5) - 0.5, animeMetal(animeHex(0xF4C24Eu)));
            } else if (style == 4) {
                AnimeMaterial m = animeMetal(a);
                float helm = max(sdCircle(p - vec2(0.0, 12.0), 38.0), p.y - 22.0);
                animePartShine(C, helm, m, animeCover(C, abs(sdCircle(p - vec2(-8.0, 2.0), 24.0)) - 2.0) * 0.6, vec3(1.0));
                animePart(C, sdLockBelly(p, vec2(0.0, -24.0), vec2(-20.0, -40.0), vec2(-40.0, -26.0), 7.0, 2.0), animeMaterial(animeHex(0xEC5662u)));
            } else if (style == 6) {
                for (int i = 0; i < 5; ++i) {
                    float x = -36.0 + float(i) * 18.0;
                    animePart(C, sdStar(p - vec2(x, 8.0 + abs(x) * 0.15), 5, 12.0, 7.0) - 1.0, animeMaterial(i % 2 == 0 ? a : animeHex(0xFFF2F6u)));
                }
            } else if (style == 3) {
                animePart(C, sdRoundBox(p - vec2(0.0, 6.0), vec2(42.0, 10.0), 8.0), main);
                animePart(C, sdLock(p, vec2(36.0, 6.0), vec2(46.0, 20.0), vec2(40.0, 40.0), 6.0, 2.0), main);
            } else if (style == 7) {
                float hood = max(sdCircle(p - vec2(0.0, 8.0), 40.0), -sdCircle(p - vec2(6.0, 18.0), 26.0));
                animePart(C, hood, main);
            } else {
                animePart(C, sdEllipse(rotate2d(p - vec2(0.0, 6.0), -0.2), vec2(44.0, 20.0)), main);
                if (style == 5) {
                    animePart(C, sdLock(p, vec2(-10.0, -6.0), vec2(-30.0, -30.0), vec2(-46.0, -34.0), 6.0, 1.0), animeMaterial(b));
                }
            }
        } else if (slot == 2) {
            // Tops: a shirt or a robe or plate.
            if (style == 2) {
                float robe = sdTrapezoid(p - vec2(0.0, 6.0), 40.0, 24.0, 40.0) - 4.0;
                animePart(C, robe, main);
                animePartNoLine(C, max(robe, abs(p.x) - 5.0), animeMaterial(b));
            } else {
                float body = sdRoundBox(p - vec2(0.0, 6.0), vec2(26.0, 32.0), 8.0);
                float sleeves = min(sdSegment(p, vec2(-24.0, -14.0), vec2(-44.0, 10.0), 10.0), sdSegment(p, vec2(24.0, -14.0), vec2(44.0, 10.0), 10.0));
                AnimeMaterial m = style == 3 ? animeMetal(a) : main;
                animePart(C, sleeves, m);
                animePart(C, body, m);
                animePart(C, max(body, abs(p.y - 26.0) - 4.0), animeMaterial(b));
            }
        } else if (slot == 3) {
            if (style == 2) {
                float skirt = sdTrapezoid(p - vec2(0.0, 6.0), 44.0, 26.0, 30.0) - 3.0;
                animePart(C, skirt, main);
                for (int i = -1; i <= 1; ++i) {
                    animeLine(C, max(sdSegment(p, vec2(float(i) * 10.0, -20.0), vec2(float(i) * 16.0, 34.0), 0.0), skirt + 1.0), main.ink);
                }
            } else {
                float len = style == 1 ? 18.0 : 40.0;
                AnimeMaterial m = style == 3 ? animeMetal(a) : main;
                animePart(C, sdRoundBox(p - vec2(0.0, -22.0), vec2(34.0, 10.0), 4.0), m);
                animePart(C, sdRoundBox(p - vec2(-17.0, -14.0 + len * 0.5), vec2(15.0, len * 0.5), 5.0), m);
                animePart(C, sdRoundBox(p - vec2(17.0, -14.0 + len * 0.5), vec2(15.0, len * 0.5), 5.0), m);
            }
        } else if (slot == 4) {
            AnimeMaterial m = style == 2 ? animeMetal(a) : main;
            for (int i = 0; i < 2; ++i) {
                vec2 o = vec2(-16.0 + float(i) * 34.0, float(i) * 4.0);
                if (style == 0 || style == 2) {
                    animePart(C, sdRoundBox(p - o - vec2(-4.0, -12.0), vec2(11.0, 16.0), 5.0), m);
                }
                animePart(C, sdRoundBox(p - o - vec2(2.0, 14.0), vec2(18.0, 10.0), 9.0), m);
            }
        } else {
            AnimeMaterial m = style == 2 ? animeMetal(a) : main;
            float mitt = sdCircle(p - vec2(4.0, -4.0), 26.0);
            mitt = animeSmoothUnion(mitt, sdCircle(p - vec2(-26.0, 6.0), 11.0), 6.0);
            mitt = animeSmoothUnion(mitt, sdRoundBox(p - vec2(4.0, 30.0), vec2(20.0, 12.0), 5.0), 4.0);
            animePart(C, mitt, m);
            animePart(C, max(mitt, abs(p.y - 28.0) - 6.0), animeMaterial(b));
        }
    }
}

void skillIcon(vec2 p, int skill, vec3 a, vec3 b, float time)
{
    // A colored disc behind every skill (its family's color), then a symbol.
    float disc = sdCircle(p, 54.0);
    animePart(C, disc, animeMaterial(mix(a, vec3(1.0), 0.15)));
    animeGloss(C, p, disc, vec2(-18.0, -24.0), vec2(22.0, 12.0), 0.35);
    AnimeMaterial ink = animeMaterial(vec3(1.0));
    AnimeMaterial gold = animeMetal(animeHex(0xF4C24Eu));
    float d = 1e5;
    if (skill == 0) {
        d = sdEllipse(p - vec2(-6.0, 8.0), vec2(20.0, 16.0));
        animePart(C, d, animeMatte(animeHex(0xC8B898u)));
        for (int i = 0; i < 3; ++i) {
            animeStroke(C, sdSegment(p, vec2(20.0 + float(i) * 6.0, -10.0 + float(i) * 8.0), vec2(36.0 + float(i) * 6.0, -10.0 + float(i) * 8.0), 0.0), 4.0,
                        vec3(1.0));
        }
        return;
    } else if (skill == 1) {
        // Catch Breath: a soft heart with a breeze.
        float heart = min(sdCircle(p - vec2(-12.0, -6.0), 15.0), sdCircle(p - vec2(12.0, -6.0), 15.0));
        heart = min(heart, sdTriangle(p, vec2(-26.0, 2.0), vec2(26.0, 2.0), vec2(0.0, 32.0)));
        animePart(C, heart, animeMaterial(animeHex(0xFF8AA8u)));
        return;
    } else if (skill == 2) {
        // Hyper Jump: a winged boot with speed lines.
        float boot = sdRoundBox(p - vec2(0.0, 10.0), vec2(18.0, 22.0), 6.0);
        boot = min(boot, sdRoundBox(p - vec2(10.0, 26.0), vec2(22.0, 9.0), 8.0));
        animePart(C, boot, animeMaterial(animeHex(0x6A9AE8u)));
        animePart(C, sdWing(p, vec2(-14.0, 0.0), 30.0, 0.6, -1.0), animeMaterial(vec3(1.0)));
        return;
    } else if (skill == 3 || skill == 4) {
        // Power Strike / Crescent Cleave: a sword with an arc.
        if (skill == 4) {
            animeStroke(C, sdArcCurve(p, 36.0, -0.8, 1.2), 7.0, vec3(1.0));
        }
        vec2 g = vec2(-20.0, 26.0);
        vec2 tip = vec2(28.0, -28.0);
        animePartShine(C, sdLock(p, g, mix(g, tip, 0.5), tip, 9.0, 2.0), animeMetal(animeHex(0xE0E8F4u)), 0.3, vec3(1.0));
        animePart(C, sdSegment(p, g + vec2(-9.0, -9.0), g + vec2(9.0, 9.0), 4.0), gold);
        if (skill == 3) {
            animePart(C, sdStar(p - vec2(14.0, -14.0), 4, 14.0, 4.0), animeGlow(vec3(1.0, 0.95, 0.7)));
        }
        return;
    } else if (skill == 5) {
        // Iron Body: a shield.
        d = sdRoundBox(p - vec2(0.0, -6.0), vec2(28.0, 22.0), 8.0);
        d = min(d, sdTriangle(p, vec2(-28.0, 6.0), vec2(28.0, 6.0), vec2(0.0, 40.0)));
        animePartShine(C, d, animeMetal(animeHex(0xC8D4E8u)), animeCover(C, abs(p.x + 8.0) - 3.0) * 0.5, vec3(1.0));
        animePart(C, sdStar(p - vec2(0.0, 2.0), 5, 12.0, 5.0), gold);
        return;
    } else if (skill == 6 || skill == 13) {
        // Masteries: a book with a star.
        d = sdRoundBox(p, vec2(30.0, 34.0), 4.0);
        animePart(C, d, animeMaterial(skill == 6 ? animeHex(0xC08A50u) : animeHex(0x7A62D8u)));
        animePart(C, sdStar(p, 5, 16.0, 7.0), gold);
        return;
    } else if (skill == 7) {
        // War Cry: a shout with bursts.
        for (int i = 0; i < 3; ++i) {
            animeStroke(C, sdArcCurve(p - vec2(-16.0, 0.0), 18.0 + float(i) * 12.0, 0.0, 0.7), 5.0, vec3(1.0));
        }
        animePart(C, sdCircle(p - vec2(-20.0, 0.0), 12.0), animeMaterial(animeHex(0xFF6A5Au)));
        return;
    } else if (skill == 8 || skill == 15) {
        // HP / MP boosts: a heart or a drop with a plus.
        if (skill == 8) {
            d = min(sdCircle(p - vec2(-13.0, -8.0), 17.0), sdCircle(p - vec2(13.0, -8.0), 17.0));
            d = min(d, sdTriangle(p, vec2(-29.0, 0.0), vec2(29.0, 0.0), vec2(0.0, 36.0)));
            animePart(C, d, animeMaterial(animeHex(0xF25F6Eu)));
        } else {
            d = sdLock(p, vec2(0.0, -36.0), vec2(-4.0, 0.0), vec2(0.0, 24.0), 1.5, 22.0);
            d = min(d, sdCircle(p - vec2(0.0, 14.0), 22.0));
            animePart(C, d, animeMaterial(animeHex(0x5E9CF2u)));
        }
        animePart(C, min(sdBox(p - vec2(22.0, 22.0), vec2(4.0, 12.0)), sdBox(p - vec2(22.0, 22.0), vec2(12.0, 4.0))), ink);
        return;
    } else if (skill == 9) {
        // Earthquake: cracked ground and a burst.
        animePart(C, sdRoundBox(p - vec2(0.0, 26.0), vec2(40.0, 12.0), 4.0), animeMatte(animeHex(0xB07A48u)));
        animeStroke(C, min(sdSegment(p, vec2(-4.0, 16.0), vec2(4.0, 30.0), 0.0), sdSegment(p, vec2(4.0, 30.0), vec2(-2.0, 38.0), 0.0)), 3.0,
                    animeHex(0x4A2A1Au));
        for (int i = 0; i < 3; ++i) {
            float x = -20.0 + float(i) * 20.0;
            animePart(C, sdTriangle(p, vec2(x - 8.0, 14.0), vec2(x + 8.0, 14.0), vec2(x, -18.0 - float(i % 2) * 10.0)), animeMatte(animeHex(0xD8B080u)));
        }
        return;
    } else if (skill == 10) {
        // Rush: a dashing arrow.
        for (int i = 0; i < 3; ++i) {
            animeStroke(C, sdSegment(p, vec2(-40.0, -14.0 + float(i) * 14.0), vec2(-14.0, -14.0 + float(i) * 14.0), 0.0), 4.0, vec3(1.0));
        }
        d = sdTriangle(p, vec2(-10.0, -26.0), vec2(-10.0, 26.0), vec2(36.0, 0.0)) - 3.0;
        animePart(C, d, animeMaterial(animeHex(0xFFD05Au)));
        return;
    } else if (skill == 11) {
        d = sdCircle(p - vec2(8.0, -4.0), 20.0);
        C.color = over(vec4(animeHex(0x8AB8FFu) * 1.4, glowFalloff(d, 14.0) * 0.7), C.color);
        animePart(C, d, animeGlow(animeHex(0xD8E8FFu)));
        animeStroke(C, sdSegment(p, vec2(-40.0, 18.0), vec2(-10.0, 4.0), 0.0), 5.0, vec3(1.0));
        return;
    } else if (skill == 12) {
        vec2 pts[4] = vec2[4](vec2(-30.0, -30.0), vec2(0.0, -4.0), vec2(-6.0, 6.0), vec2(28.0, 32.0));
        d = min(min(sdSegment(p, pts[0], pts[1], 5.0), sdSegment(p, pts[1], pts[2], 5.0)), sdSegment(p, pts[2], pts[3], 5.0));
        C.color = over(vec4(animeHex(0xB8F0FFu) * 1.4, glowFalloff(d, 10.0) * 0.8), C.color);
        animePart(C, d, animeGlow(animeHex(0xF0FCFFu)));
        return;
    } else if (skill == 14) {
        // Mana Shield: a bubble.
        d = abs(sdCircle(p, 34.0)) - 4.0;
        C.color = over(vec4(animeHex(0x9A8CFFu) * 1.3, glowFalloff(d, 10.0) * 0.6), C.color);
        animePart(C, d, animeGlow(animeHex(0xD0C8FFu)));
        animeFlat(C, sdCircle(p - vec2(-12.0, -14.0), 6.0), vec3(1.0));
        return;
    } else if (skill == 16) {
        // Swiftwind: swirls.
        for (int i = 0; i < 3; ++i) {
            animeStroke(C, sdArcCurve(p - vec2(-6.0 + float(i) * 6.0, float(i) * 6.0 - 6.0), 16.0 + float(i) * 8.0, 3.8 + float(i) * 0.3, 1.4), 5.0,
                        vec3(1.0));
        }
        return;
    } else if (skill == 17) {
        // Starfall: three falling stars.
        for (int i = 0; i < 3; ++i) {
            vec2 at = vec2(-22.0 + float(i) * 22.0, -14.0 + float(i % 2) * 22.0);
            animeStroke(C, sdSegment(p, at + vec2(-14.0, -18.0), at, 0.0), 4.0, vec3(1.0));
            animePart(C, sdStar(p - at, 5, 12.0, 5.5) - 0.5, gold);
        }
        return;
    } else if (skill == 18) {
        // Bloom: a flower.
        for (int i = 0; i < 5; ++i) {
            float ang = float(i) * 1.2566;
            animePart(C, sdCircle(p - vec2(cos(ang), sin(ang)) * 18.0, 14.0), animeMaterial(animeHex(0xFF9AC8u)));
        }
        animePart(C, sdCircle(p, 11.0), animeMaterial(animeHex(0xFAD65Eu)));
        return;
    }
}

void statIcon(vec2 p, int id)
{
    // 0 strength (a fist), 1 agility (a feather), 2 intellect (a star book),
    // 3 vitality (a heart), 4 hp, 5 mp, 6 speed, 7 jump, 8 attack, 9 defense,
    // 10 critical, 11 magic.
    if (id == 0 || id == 8) {
        float fist = sdRoundBox(p, vec2(30.0, 26.0), 14.0);
        animePart(C, fist, animeSkin(animeHex(0xFAD6BCu)));
        for (int i = 0; i < 3; ++i) {
            animeLine(C, max(abs(p.x + 10.0 - float(i) * 10.0), max(fist + 2.0, p.y + 6.0)), animeInkOf(animeHex(0xFAD6BCu)));
        }
        if (id == 8) {
            animePart(C, sdLock(p, vec2(20.0, 20.0), vec2(36.0, -10.0), vec2(44.0, -40.0), 6.0, 1.0), animeMetal(animeHex(0xE0E8F4u)));
        }
    } else if (id == 1 || id == 6) {
        animePart(C, sdLock(p, vec2(-30.0, 40.0), vec2(4.0, -6.0), vec2(30.0, -40.0), 3.0, 14.0), animeMaterial(animeHex(0x7CCB8Au)));
        animeLine(C, sdBezier(p, vec2(-28.0, 38.0), vec2(4.0, -4.0), vec2(26.0, -34.0)), animeInkOf(animeHex(0x7CCB8Au)));
    } else if (id == 2 || id == 11) {
        float star = sdStar(p, 5, 40.0, 18.0) - 2.0;
        animePart(C, star, animeMaterial(animeHex(0xA06CE4u)));
    } else if (id == 3 || id == 4) {
        float d = min(sdCircle(p - vec2(-15.0, -10.0), 20.0), sdCircle(p - vec2(15.0, -10.0), 20.0));
        d = min(d, sdTriangle(p, vec2(-33.0, 0.0), vec2(33.0, 0.0), vec2(0.0, 40.0)));
        animePart(C, d, animeMaterial(animeHex(0xF25F6Eu)));
    } else if (id == 5) {
        float d = sdLock(p, vec2(0.0, -40.0), vec2(-4.0, 0.0), vec2(0.0, 28.0), 1.5, 26.0);
        d = min(d, sdCircle(p - vec2(0.0, 16.0), 26.0));
        animePart(C, d, animeMaterial(animeHex(0x5E9CF2u)));
    } else if (id == 7) {
        animePart(C, sdTriangle(p, vec2(-30.0, 10.0), vec2(30.0, 10.0), vec2(0.0, -34.0)) - 4.0, animeMaterial(animeHex(0x8CDCFFu)));
        animePart(C, sdRoundBox(p - vec2(0.0, 26.0), vec2(10.0, 16.0), 3.0), animeMaterial(animeHex(0x8CDCFFu)));
    } else if (id == 9) {
        float d = sdRoundBox(p - vec2(0.0, -8.0), vec2(30.0, 24.0), 8.0);
        d = min(d, sdTriangle(p, vec2(-30.0, 6.0), vec2(30.0, 6.0), vec2(0.0, 42.0)));
        animePart(C, d, animeMetal(animeHex(0xC8D4E8u)));
    } else {
        animePart(C, sdStar(p, 4, 42.0, 10.0), animeGlow(animeHex(0xFFE68Au)));
    }
}

// The world map's painted ground: a soft sea with waves and the lands as
// rounded shapes (the game draws the paths and pins over it).
vec4 worldMap(vec2 p, vec2 halfSize, float time)
{
    // p in canvas units relative to the map's center.
    vec2 u = p / halfSize; // -1..1
    vec3 sea = mix(animeHex(0x8AD4F2u), animeHex(0x5EB4E8u), smoothstep(-1.0, 1.0, u.y));
    float wave = abs(fract((p.y + sin(p.x * 0.02 + time * 0.5) * 6.0) / 28.0) - 0.5);
    sea = mix(sea, animeHex(0xC8ECFFu), smoothstep(0.06, 0.0, wave) * 0.35);
    vec4 c = vec4(sea, 1.0);
    // The lands: Petal Isle (left), the mainland (right) with the forest,
    // mountains in the north and the harbor.
    float isle = sdEllipse(p - vec2(-halfSize.x * 0.62, halfSize.y * 0.28), vec2(halfSize.x * 0.24, halfSize.y * 0.3));
    isle = animeSmoothUnion(isle, sdEllipse(p - vec2(-halfSize.x * 0.72, halfSize.y * 0.05), vec2(halfSize.x * 0.12, halfSize.y * 0.18)), 30.0);
    float main = sdEllipse(p - vec2(halfSize.x * 0.32, -halfSize.y * 0.05), vec2(halfSize.x * 0.62, halfSize.y * 0.82));
    main = animeSmoothUnion(main, sdEllipse(p - vec2(-halfSize.x * 0.1, halfSize.y * 0.5), vec2(halfSize.x * 0.26, halfSize.y * 0.22)), 40.0);
    float land = min(isle, main);
    float px = 1.0;
    c = over(vec4(animeHex(0xF6E8C0u), clamp(0.5 - (land - 10.0) / px, 0.0, 1.0)), c); // sand rim
    vec3 grass = animeHex(0x9AD87Au);
    // Regions tinted: forest east, mountain north, depths under the harbor.
    vec3 tone = grass;
    tone = mix(tone, animeHex(0x6CB866u), smoothstep(-0.1, 0.4, u.x) * (1.0 - smoothstep(-0.6, -0.2, -u.y)) * 0.0 + smoothstep(0.1, 0.5, u.x) * 0.6);
    tone = mix(tone, animeHex(0xE6EEF8u), smoothstep(-0.25, -0.65, u.y) * smoothstep(-0.2, 0.3, u.x));
    c = over(vec4(tone, clamp(0.5 - land / px, 0.0, 1.0)), c);
    c = over(vec4(animeInkOf(grass) * 2.0, clamp(0.5 - (abs(land) - 1.6) / px, 0.0, 1.0) * 0.7), c);
    return c;
}

void main()
{
    SpriteData s = sprite;
    vec2 local = spriteLocal(inUv);
    int kind = int(s.params.x + 0.5);
    vec3 a = s.color.rgb;
    vec3 b = s.color2.rgb;
    float time = s.time;
    vec2 halfSize = max(s.size * 0.5 - kAnimeUiMargin, vec2(1.0));
    bool panel = kind == kWindow || kind == kButton || kind == kSlot || kind == kGauge || kind == kKeyCap || kind == kWell || kind == kArrow
                 || kind == kTooltip || kind == kTab || kind == kPlate || kind == kSweep;
    if (kind == kMap) {
        outColor = worldMap(local, s.size * 0.5, time);
        outColor.a *= s.color.a;
        return;
    }
    if (kind == kShadow) {
        float d = sdRoundBox(local, s.size * 0.5 - 20.0, 20.0);
        outColor = vec4(kAnimeUiShadow, (1.0 - smoothstep(-10.0, 20.0, d)) * 0.35 * s.color.a);
        return;
    }
    if (panel) {
        C = animeBegin(local, 2.4);
        C.shadeDepth = min(halfSize.y * 0.35, 8.0);
        if (kind == kWindow) {
            animeUiWindow(C, local, halfSize, s.params.w > 0.0 ? s.params.w : 18.0, s.params.y, int(s.params.z + 0.5), a, b);
        } else if (kind == kWell) {
            animeUiWindow(C, local, halfSize, s.params.w > 0.0 ? s.params.w : 12.0, 0.0, kAnimeUiWell, a, b);
        } else if (kind == kTooltip) {
            animeUiWindow(C, local, halfSize, s.params.w > 0.0 ? s.params.w : 14.0, 0.0, kAnimeUiTooltip, a, b);
        } else if (kind == kButton) {
            animeUiButton(C, local, halfSize, int(s.params.y + 0.5), a, b);
        } else if (kind == kSlot) {
            animeUiSlot(C, local, halfSize, s.params.w > 0.0 ? s.params.w : 10.0, clamp(s.params.y, 0.0, 1.0), clamp(s.params.z, 0.0, 1.0), a, b);
        } else if (kind == kGauge) {
            animeUiGauge(C, local, halfSize, s.params.y, s.params.z, a, b);
        } else if (kind == kKeyCap) {
            animeUiKeyCap(C, local, halfSize, a);
        } else if (kind == kArrow) {
            animeUiArrow(C, local, halfSize, a);
        } else if (kind == kTab) {
            // A tab: a rounded-top pill, lifted and brighter when selected.
            bool selected = s.params.y > 0.5;
            float d = sdRoundBox(local - vec2(0.0, selected ? 0.0 : 3.0), halfSize - vec2(0.0, selected ? 0.0 : 3.0), min(halfSize.y, 14.0));
            d = max(d, local.y - halfSize.y);
            vec3 fill = selected ? a : mix(a, b, 0.35) * 0.92;
            C.color = over(vec4(fill, animeCover(C, d)), C.color);
            C.color = over(vec4(animeInkOf(b), animeUiLine(C, abs(d), 2.4)), C.color);
            C.color = over(vec4(vec3(1.0), animeUiGloss(C, local, vec2(-halfSize.x + 10.0, -halfSize.y + 6.0), vec2(halfSize.x - 10.0, -halfSize.y + 6.0), 3.0) * 0.4), C.color);
        } else if (kind == kPlate) {
            // A name plate: a soft pill with a darker fill and a light border.
            float d = sdRoundBox(local, halfSize, halfSize.y);
            C.color = over(vec4(kAnimeUiShadow, (1.0 - smoothstep(-2.0, 6.0, sdRoundBox(local - vec2(0.0, 3.0), halfSize, halfSize.y))) * 0.3), C.color);
            C.color = over(vec4(a, animeCover(C, d) * s.color.a), C.color);
            C.color = over(vec4(b, animeUiLine(C, abs(d + 1.5), 2.0)), C.color);
        } else {
            // The cooldown sweep: a veil over the part of the slot still
            // cooling down, clockwise from the top, with a bright edge.
            float remaining = clamp(s.params.y, 0.0, 1.0);
            float d = sdRoundBox(local, halfSize, s.params.w > 0.0 ? s.params.w : 10.0);
            float ang = atan(local.x, -local.y); // 0 at the top, clockwise
            float f = fract(ang / 6.2832 + 1.0);
            float veil = step(1.0 - remaining, f) * animeCover(C, d);
            C.color = over(vec4(vec3(0.05, 0.04, 0.12), veil * 0.62), C.color);
            float edgeA = (1.0 - remaining) * 6.2832;
            vec2 dir = vec2(sin(edgeA), -cos(edgeA));
            float line = abs(dot(local, vec2(-dir.y, dir.x))) - 1.2;
            float onRay = step(0.0, dot(local, dir));
            C.color = over(vec4(b * 1.4, animeCover(C, line) * onRay * animeCover(C, d) * step(0.001, remaining) * 0.9), C.color);
        }
        outColor = animeFinish(C);
        outColor.a *= s.color.a;
        return;
    }
    vec2 p = local / (s.size.y / 128.0);
    C = animeBegin(p, 4.5);
    C.shadeDepth = 9.0;
    if (kind == kMarker) {
        animeUiMarker(C, p, int(s.params.y + 0.5), a);
    } else if (kind == kDigit) {
        animeUiDigit(C, p, int(s.params.y + 0.5), a, b);
    } else if (kind == kItemIcon) {
        itemIcon(p, int(s.params.y + 0.5), int(s.params.z + 0.5), a, b, time);
    } else if (kind == kSkillIcon) {
        skillIcon(p, int(s.params.y + 0.5) - 80, a, b, time);
    } else if (kind == kStatIcon) {
        statIcon(p, int(s.params.y + 0.5));
    } else if (kind == kCoin) {
        float d = sdCircle(p, 44.0);
        animePart(C, d, animeMetal(animeHex(0xF4C24Eu)));
        animePart(C, sdStar(p, 5, 24.0, 11.0), animeMetal(animeHex(0xFFE08Au)));
        animeGloss(C, p, d, vec2(-14.0, -16.0), vec2(12.0, 8.0), 0.5);
    } else if (kind == kBadge) {
        // The level badge: a round gold medallion with a ribbon edge.
        float d = sdCircle(p, 56.0);
        animePart(C, d, animeMetal(animeHex(0xF4C24Eu)));
        animePart(C, sdCircle(p, 44.0), animeMaterial(a));
        animeGloss(C, p, d, vec2(-18.0, -26.0), vec2(20.0, 10.0), 0.4);
    } else if (kind == kLogo) {
        // The title's ribbon banner behind the logo text.
        vec2 q = p;
        float bend = q.y + 0.0004 * q.x * q.x - 12.0;
        float ribbon = max(abs(bend) - 26.0, abs(q.x) - 300.0);
        animePart(C, ribbon, animeMaterial(a));
        animeGloss(C, p, ribbon, vec2(-120.0, -10.0), vec2(160.0, 8.0), 0.3);
    }
    outColor = animeFinish(C);
    outColor.a *= s.color.a;
}
