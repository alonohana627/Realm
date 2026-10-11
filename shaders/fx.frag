#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime.glsl"

// Realm's effects: swing arcs, the crescent cleave, stars, hit sparks,
// petals, wind swirls, portals, shockwaves, warnings, hostile shots, the
// level-up pillar, coins, loot glows, dust puffs, glowing streaks, heal
// pluses, arcane bolts, lightning, shields, rings, roots, beams, glows,
// leaves and snowflakes. Soft, bright and outlined like the rest of the
// world, glowing above 1 for the bloom pass.
//
// The quad is the effect's box; u is the pixel in -1..1 over it.
//   params  x kind (app Fx), y progress 0..1, z variant, w seed
//   color   core color          color2  edge color

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

const int kSlash = 0, kCrescent = 1, kStar = 2, kSpark = 3, kPetal = 4, kWind = 5, kPortal = 6, kShockwave = 7, kWarn = 8,
          kShot = 9, kLevelUp = 10, kCoin = 11, kLootGlow = 12, kPuff = 13, kStreak = 14, kPlus = 15, kBolt = 16,
          kLightning = 17, kShield = 18, kRing = 19, kRoot = 20, kBeam = 21, kGlow = 22, kLeaf = 23, kSnow = 24;

float px;

float cover(float d)
{
    return clamp(0.5 - d / px, 0.0, 1.0);
}

vec4 outlined(float d, vec3 fill, vec3 ink, float line)
{
    vec4 c = vec4(fill, cover(d));
    c = over(vec4(ink, cover(abs(d) - line * 0.5) * c.a), c);
    return c;
}

void main()
{
    SpriteData s = sprite;
    vec2 local = spriteLocal(inUv);
    vec2 u = local / (s.size * 0.5); // -1..1
    px = length(dFdx(u)) * 1.2;
    int kind = int(s.params.x + 0.5);
    float t = clamp(s.params.y, 0.0, 1.0);
    float variant = s.params.z;
    float seed = s.params.w;
    float time = s.time;
    vec3 core = s.color.rgb;
    vec3 edge = s.color2.rgb;
    float alpha = s.color.a;
    vec4 c = vec4(0.0);

    if (kind == kSlash || kind == kCrescent) {
        // A crescent smear: thick in the middle, tapering at both ends,
        // sweeping on (t < 0.4) then fading. Variant: the arc's span.
        float span = kind == kCrescent ? 2.6 : 1.9 + variant * 0.3;
        float sweep = smoothstep(0.0, 0.45, t);
        float a = atan(u.y, u.x);
        float start = -span * 0.5 - 0.6;
        float end = start + span * sweep;
        float along = clamp((a - start) / max(end - start, 1e-3), 0.0, 1.0);
        float inArc = step(start, a) * step(a, end);
        float r = length(u);
        float thick = 0.2 * sin(along * kAnimePi) * (1.0 - smoothstep(0.55, 1.0, t)) + 0.01;
        float d = abs(r - 0.72) - thick;
        float body = cover(d) * inArc;
        vec3 col = mix(edge, core * 1.4, smoothstep(0.0, 1.0, along));
        col = mix(col, vec3(1.4), cover(abs(r - 0.72 - thick * 0.3) - thick * 0.25) * 0.6);
        c = vec4(col, body * (1.0 - smoothstep(0.6, 1.0, t)));
        c = over(c, vec4(edge, glowFalloff(d, 0.12) * inArc * 0.4 * (1.0 - t)));
    } else if (kind == kStar) {
        float spin = seed + time * 2.0;
        vec2 q = rotate2d(u, spin);
        float d = sdStar(q, 5, 0.62, 0.29) - 0.06;
        c = outlined(d, core, edge * 0.6, 0.05);
        c.rgb = mix(c.rgb, vec3(1.4), cover(sdStar(q - vec2(-0.08, -0.1), 5, 0.25, 0.12)) * 0.6);
        c = over(c, vec4(edge, glowFalloff(d, 0.3) * 0.5));
    } else if (kind == kSpark) {
        // A hit spark: a bright burst with rays, quick.
        float r = length(u);
        float a = atan(u.y, u.x) + seed;
        float rays = pow(abs(sin(a * 4.0)), 8.0);
        float grow = smoothstep(0.0, 0.25, t);
        float burst = (1.0 - smoothstep(0.1, 0.25 + 0.6 * grow, r / (0.4 + rays * 0.6))) * (1.0 - smoothstep(0.4, 1.0, t));
        float ring = cover(abs(r - 0.3 - t * 0.6) - 0.04 * (1.0 - t));
        c = vec4(mix(core, vec3(1.5), burst * 0.5), max(burst, ring * (1.0 - t)));
        c.rgb = mix(c.rgb, edge * 1.3, ring * 0.5);
    } else if (kind == kPetal) {
        vec2 q = rotate2d(u, seed + time * 3.0);
        float d = sdEllipse(q, vec2(0.75, 0.45));
        c = outlined(d, core, edge * 0.7, 0.08);
    } else if (kind == kWind) {
        // A swirl of streaks around the hero (a buff).
        float r = length(u);
        float a = atan(u.y, u.x);
        float swirl = 0.0;
        for (int i = 0; i < 3; ++i) {
            float fi = float(i);
            float aa = a - time * (3.0 + fi) - fi * 2.1;
            float band = abs(r - 0.55 - 0.15 * fi - 0.05 * sin(aa * 2.0));
            float arc = smoothstep(0.0, 1.2, fract(aa / 6.2832)) * (1.0 - smoothstep(0.7, 1.0, fract(aa / 6.2832)));
            swirl = max(swirl, cover(band - 0.025) * arc);
        }
        c = vec4(mix(edge, core, 0.5) * 1.2, swirl * 0.8);
    } else if (kind == kPortal) {
        // A swirling MapleStory-ish portal: a glowing ellipse of light with
        // spiraling bands and rising motes. variant: 0 open, 1 closed.
        vec2 q = u * vec2(1.0, 0.62);
        float r = length(q);
        float a = atan(q.y, q.x);
        float open = 1.0 - clamp(variant, 0.0, 1.0);
        float body = 1.0 - smoothstep(0.55, 0.62, r);
        float spiral = 0.5 + 0.5 * sin(a * 3.0 - r * 14.0 + time * 4.0);
        vec3 col = mix(edge, core * 1.3, spiral * body);
        col = mix(col, vec3(1.6), (1.0 - smoothstep(0.0, 0.25, r)) * 0.8);
        float rim = cover(abs(r - 0.6) - 0.03);
        c = vec4(col, body * mix(0.35, 0.9, open));
        c = over(vec4(edge * 1.6, rim), c);
        c = over(c, vec4(edge, glowFalloff(r - 0.6, 0.25) * 0.5 * open));
        for (int i = 0; i < 6; ++i) {
            float fi = float(i);
            float tt = fract(time * 0.4 + fi / 6.0 + seed);
            vec2 at = vec2(sin(fi * 2.4 + seed) * 0.4, 0.5 - tt * 1.3);
            c = over(vec4(vec3(1.4), cover(length(u - at) - 0.04 * (1.0 - tt)) * open), c);
        }
    } else if (kind == kShockwave) {
        // A ground wave: a bright half-ring rising along the floor.
        float r = length(u * vec2(1.0, 1.8));
        float ring = cover(abs(r - 0.3 - t * 0.6) - 0.12 * (1.0 - t)) * step(u.y, 0.2);
        c = vec4(mix(core, vec3(1.3), 0.4), ring * (1.0 - t));
        c = over(c, vec4(edge, glowFalloff(abs(r - 0.3 - t * 0.6) - 0.1, 0.2) * (1.0 - t) * 0.5 * step(u.y, 0.2)));
    } else if (kind == kWarn) {
        // A telegraph: a soft column with a pulsing marker on the floor.
        float col = (1.0 - smoothstep(0.2, 0.55, abs(u.x))) * smoothstep(-1.0, 0.8, u.y) * 0.35;
        float mark = cover(abs(length(u * vec2(1.0, 3.0) - vec2(0.0, 2.4)) - 0.5 + 0.1 * sin(time * 12.0)) - 0.06);
        float bang = cover(min(sdBox(u - vec2(0.0, -0.1), vec2(0.07, 0.3)), length(u - vec2(0.0, 0.32)) - 0.08));
        float blink = 0.6 + 0.4 * sin(time * 14.0);
        c = vec4(core, col * (0.4 + t));
        c = over(vec4(edge * 1.3, mark * blink), c);
        c = over(vec4(vec3(1.0), bang * blink * step(0.3, t)), c);
    } else if (kind == kShot) {
        // A hostile shot by look: 0 acorn, 1 snowball, 2 gloom orb, 3 ground
        // wave, 4 bolt, 5 star, 6 pebble.
        int look = int(variant + 0.5);
        float spin = time * 6.0 + seed;
        if (look == 0) {
            vec2 q = rotate2d(u, spin * 0.5);
            float nut = sdEllipse(q - vec2(0.0, 0.15), vec2(0.42, 0.5));
            float capD = max(sdEllipse(q - vec2(0.0, -0.2), vec2(0.5, 0.3)), q.y + 0.05);
            c = outlined(nut, animeHex(0xC8884Au), animeHex(0x5A3A22u), 0.08);
            c = over(outlined(capD, animeHex(0x8A5A3Au), animeHex(0x4A2A1Au), 0.08), c);
            c = over(outlined(sdSegment(q, vec2(0.0, -0.45), vec2(0.08, -0.65), 0.05), animeHex(0x6A4A2Au), animeHex(0x3A2A1Au), 0.06), c);
        } else if (look == 1) {
            float d = length(u) - 0.55;
            c = outlined(d, vec3(0.97, 0.98, 1.0), animeHex(0x7A9AC8u), 0.08);
            c.rgb = mix(c.rgb, animeHex(0xC8DCF4u), smoothstep(-0.2, 0.3, u.x + u.y) * c.a * 0.5);
        } else if (look == 2) {
            float d = length(u) - 0.45 - 0.06 * sin(atan(u.y, u.x) * 5.0 + time * 6.0);
            c = vec4(core, glowFalloff(d, 0.4) * 0.7);
            c = over(outlined(d, mix(core, vec3(0.1, 0.05, 0.2), 0.4), edge, 0.08), c);
            c.rgb = mix(c.rgb, vec3(1.3), cover(length(u + vec2(0.15, 0.15)) - 0.12) * 0.6);
        } else if (look == 3) {
            float d = max(abs(u.x) - 0.45, -u.y - 0.7 + abs(u.x) * 0.6);
            d = max(d, u.y - 0.6);
            c = vec4(mix(core, vec3(1.3), 0.3), cover(d) * 0.85);
            c = over(c, vec4(edge, glowFalloff(d, 0.3) * 0.5));
        } else if (look == 6) {
            vec2 q = rotate2d(u, spin);
            float d = sdEllipse(q, vec2(0.5, 0.42));
            c = outlined(d, animeHex(0xB8AC98u), animeHex(0x5A5040u), 0.09);
            c.rgb = mix(c.rgb, vec3(1.0), cover(length(q + vec2(0.15, 0.15)) - 0.1) * 0.6);
        } else {
            float d = length(u) - 0.4;
            c = vec4(core, glowFalloff(d, 0.5) * 0.8);
            c = over(vec4(mix(core, vec3(1.5), 0.5), cover(d)), c);
        }
    } else if (kind == kLevelUp) {
        // A pillar of light with rising sparkles and a ring at the feet.
        float fade = 1.0 - smoothstep(0.6, 1.0, t);
        float grow = smoothstep(0.0, 0.2, t);
        float beam = (1.0 - smoothstep(0.1, 0.45 * grow, abs(u.x))) * smoothstep(1.0, 0.2, u.y) * fade;
        c = vec4(mix(edge, core * 1.4, beam), beam * 0.7);
        for (int i = 0; i < 10; ++i) {
            float fi = float(i);
            float tt = fract(t * 1.6 + fi * 0.1);
            vec2 at = vec2(sin(fi * 2.7) * 0.45, 0.9 - tt * 1.8);
            float d = sdStar(u - at, 4, 0.07 * (1.0 - tt) + 0.02, 0.02);
            c = over(vec4(vec3(1.5, 1.4, 1.0), cover(d) * fade), c);
        }
        float ring = cover(abs(length((u - vec2(0.0, 0.85)) * vec2(1.0, 4.0)) - 0.3 - t * 0.6) - 0.05);
        c = over(vec4(core * 1.3, ring * fade), c);
    } else if (kind == kCoin) {
        // A spinning gold coin.
        float spin = cos(t * 6.2832);
        vec2 q = u / vec2(max(abs(spin), 0.15), 1.0);
        float d = length(q) - 0.6;
        c = outlined(d, mix(core, core * 0.8, step(0.0, spin) * 0.0), animeHex(0x9A6A10u), 0.1);
        c.rgb = mix(c.rgb, vec3(1.0, 0.98, 0.8) * 1.2, cover(length(q - vec2(-0.2, -0.2)) - 0.18) * 0.8);
        c = over(vec4(c.rgb, c.a), vec4(core, glowFalloff(d, 0.25) * 0.4));
        float star = sdStar(q, 5, 0.28, 0.13);
        c.rgb = mix(c.rgb, animeHex(0xE0A020u), cover(star) * 0.7 * c.a);
    } else if (kind == kLootGlow) {
        // A soft glow under a drop by rarity, with a beam for epic and up.
        float r = length(u * vec2(1.0, 1.8) - vec2(0.0, 0.3));
        c = vec4(core, (1.0 - smoothstep(0.0, 0.8, r)) * 0.5 * alpha);
        if (variant > 0.5) {
            float beam = (1.0 - smoothstep(0.0, 0.18, abs(u.x))) * smoothstep(0.6, -1.0, u.y) * (0.6 + 0.4 * sin(time * 3.0 + seed));
            c = over(vec4(edge, beam * 0.6), c);
        }
        alpha = 1.0;
    } else if (kind == kPuff) {
        float r = length(u);
        float d = r - 0.5 - 0.1 * sin(atan(u.y, u.x) * 5.0 + seed);
        c = outlined(d, core, edge, 0.08);
        c.a *= 0.85;
    } else if (kind == kStreak) {
        // A glowing tail: brightest at the right end, fading left.
        float along = u.x * 0.5 + 0.5;
        float w = 0.08 + 0.4 * along;
        float d = abs(u.y) - w;
        c = vec4(mix(edge, core * 1.3, along), cover(d) * along * along);
    } else if (kind == kPlus) {
        float d = min(sdBox(u, vec2(0.18, 0.55)), sdBox(u, vec2(0.55, 0.18))) - 0.05;
        c = outlined(d, core, edge, 0.1);
    } else if (kind == kBolt) {
        // An arcane bolt: a glowing orb with orbiting sparks.
        float r = length(u);
        c = vec4(edge, glowFalloff(r - 0.3, 0.5) * 0.8);
        c = over(vec4(mix(core, vec3(1.6), 0.5), cover(r - 0.32)), c);
        for (int i = 0; i < 3; ++i) {
            float a = time * 8.0 + float(i) * 2.094 + seed;
            vec2 at = vec2(cos(a), sin(a)) * 0.55;
            c = over(vec4(vec3(1.5), cover(length(u - at) - 0.08)), c);
        }
    } else if (kind == kLightning) {
        // A jagged bolt along the quad's x (the game rotates and stretches it).
        float x = u.x * 0.5 + 0.5;
        float jag = (hash12(uvec2(uint(x * 12.0 + seed * 7.0), uint(time * 20.0))) - 0.5) * 0.6
                    + sin(x * 30.0 + time * 40.0) * 0.12;
        float d = abs(u.y - jag * (1.0 - abs(u.x))) - 0.06;
        float fade = 1.0 - t;
        c = vec4(edge, glowFalloff(d, 0.25) * fade * 0.8);
        c = over(vec4(vec3(1.6), cover(d) * fade), c);
    } else if (kind == kShield) {
        float r = length(u);
        float rim = glowFalloff(abs(r - 0.85) - 0.02, 0.15);
        float hex = abs(fract(u.x * 5.0 + floor(u.y * 5.0) * 0.5) - 0.5) + abs(fract(u.y * 5.0) - 0.5);
        c = vec4(core, (1.0 - smoothstep(0.84, 0.88, r)) * (0.12 + 0.1 * smoothstep(0.6, 0.7, hex)));
        c = over(vec4(edge * 1.3, rim * (0.6 + 0.2 * sin(time * 3.0))), c);
    } else if (kind == kRing) {
        float r = length(u);
        float d = abs(r - 0.3 - t * 0.65) - 0.06 * (1.0 - t);
        c = vec4(mix(core, vec3(1.3), 0.3), cover(d) * (1.0 - t));
        c = over(c, vec4(edge, glowFalloff(d, 0.15) * (1.0 - t) * 0.6));
    } else if (kind == kRoot) {
        // A thorny root bursting out of the floor (the quad's bottom).
        float rise = smoothstep(0.0, 0.25, t) * (1.0 - smoothstep(0.75, 1.0, t));
        float topY = 1.0 - rise * 1.9;
        vec2 q = u;
        q.x += sin(q.y * 4.0 + seed) * 0.08;
        float d = max(abs(q.x) - 0.28 * (q.y - topY) / (1.0 - topY + 1e-3) - 0.04, topY - q.y);
        for (int i = 0; i < 3; ++i) {
            float y = topY + 0.4 + float(i) * 0.4;
            float sx = i % 2 == 0 ? 1.0 : -1.0;
            d = min(d, sdTriangle(q, vec2(0.0, y), vec2(0.0, y + 0.2), vec2(sx * 0.4, y - 0.05)));
        }
        c = outlined(d, core, animeInkOf(core) * 2.0, 0.06);
        c.rgb = mix(c.rgb, edge, smoothstep(0.2, -0.3, q.x) * 0.3 * c.a);
    } else if (kind == kBeam) {
        float beam = (1.0 - smoothstep(0.0, 0.6, abs(u.x))) * (1.0 - t);
        c = vec4(mix(edge, core * 1.4, beam), beam * 0.8);
    } else if (kind == kGlow) {
        float r = length(u);
        c = vec4(core, (1.0 - smoothstep(0.0, 1.0, r)) * alpha);
        alpha = 1.0;
    } else if (kind == kLeaf) {
        vec2 q = rotate2d(u, seed + time * 2.0);
        float d = sdLock(q, vec2(-0.6, 0.0), vec2(0.0, -0.35), vec2(0.6, 0.0), 0.32, 0.02);
        c = outlined(d, core, edge, 0.08);
        c = over(vec4(edge, cover(max(abs(q.y + 0.05 * q.x) - 0.02, abs(q.x) - 0.5)) * c.a), c);
    } else {
        // A snowflake.
        vec2 q = rotate2d(u, seed + time);
        float d = 1e5;
        for (int i = 0; i < 3; ++i) {
            d = min(d, sdSegment(rotate2d(q, float(i) * 1.047), vec2(-0.7, 0.0), vec2(0.7, 0.0), 0.08));
        }
        c = vec4(vec3(1.0), cover(d) * 0.9);
    }
    c.a *= alpha;
    outColor = c;
}
