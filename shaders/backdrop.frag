#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime.glsl"

// Realm's parallax backdrops: one full-canvas quad per layer (draw::
// drawParallax), the scroll offset in params.xy (canvas units).
//   params.z  layer: 0 sky, 1 clouds, 2 far, 3 near, 4 light and mist
//   params.w  night 0..1
//   bits.x    scene: 0 meadow isle, 1 sea and harbor, 2 giant forest,
//             3 snowy peaks, 4 cave depths, 5 glowing hollow
//   color / color2  the layer's two colors (sky: horizon / top; far and
//             near: body / rim)
// Pastel, soft, a little hazy, with faint ink outlines so the backdrop and
// the characters are one drawing, but quieter.

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

float h1(float x)
{
    return fract(sin(x * 127.1) * 43758.5453);
}

// A soft rounded ridge line: y of the silhouette top at x.
float ridge(float x, float base, float amp, float scale, float seed)
{
    return base - amp * (0.55 * sin(x * scale + seed) + 0.3 * sin(x * scale * 2.3 + seed * 1.7) + 0.15 * sin(x * scale * 5.1 + seed * 0.3));
}

// A silhouette with a faint outline and a lighter rim along its top.
vec4 silhouette(float d, vec3 body, vec3 rim, float px, float rimWidth, float inkAlpha)
{
    float cover = clamp(0.5 - d / px, 0.0, 1.0);
    vec3 col = mix(body, rim, smoothstep(-rimWidth, 0.0, d) * 0.85);
    vec4 c = vec4(col, cover);
    float line = clamp(0.5 - (abs(d) - 1.6) / px, 0.0, 1.0);
    c = over(vec4(animeInkOf(body) * 2.2 + body * 0.35, line * inkAlpha), c);
    return c;
}

// Puffy cloud: circles along a line, flat-ish bottom, two-tone.
float cloudShape(vec2 q, vec2 at, float size, float seed)
{
    float d = 1e5;
    for (int i = 0; i < 5; ++i) {
        float fi = float(i);
        vec2 c = at + vec2((fi - 2.0) * size * 0.55, -size * (0.25 + 0.3 * sin(fi * 2.1 + seed)) * (1.0 - abs(fi - 2.0) * 0.25));
        d = min(d, length(q - c) - size * (0.45 + 0.18 * h1(fi + seed)));
    }
    d = max(d, q.y - at.y - size * 0.18);
    return d;
}

// A broadleaf tree silhouette (trunk and round crown) at x.
float treeShape(vec2 q, float x, float ground, float size, float seed)
{
    float trunk = sdBox(q - vec2(x, ground - size * 0.35), vec2(size * 0.07, size * 0.35));
    float crown = 1e5;
    for (int i = 0; i < 4; ++i) {
        float fi = float(i);
        vec2 c = vec2(x + (fi - 1.5) * size * 0.22, ground - size * (0.85 + 0.12 * sin(fi * 1.9 + seed)));
        crown = min(crown, length(q - c) - size * (0.3 + 0.06 * h1(fi + seed)));
    }
    return min(trunk, crown);
}

float pineShape(vec2 q, float x, float ground, float size)
{
    float d = 1e5;
    for (int i = 0; i < 3; ++i) {
        float fi = float(i);
        float y0 = ground - size * (0.15 + fi * 0.28);
        d = min(d, sdTriangle(q, vec2(x - size * (0.36 - fi * 0.08), y0), vec2(x + size * (0.36 - fi * 0.08), y0),
                              vec2(x, y0 - size * 0.42)));
    }
    return min(d - size * 0.03, sdBox(q - vec2(x, ground - size * 0.08), vec2(size * 0.05, size * 0.1)));
}

void main()
{
    SpriteData s = sprite;
    vec2 local = spriteLocal(inUv);
    vec2 q0 = local + s.size * 0.5; // canvas units, top-left origin
    vec2 scroll = s.params.xy;
    vec2 q = q0 + scroll;
    int layer = int(s.params.z + 0.5);
    float night = clamp(s.params.w, 0.0, 1.0);
    int scene = int(s.bits.x & 7u);
    float time = s.time;
    float px = 1.0;
    float H = s.size.y;

    if (layer == 0) {
        // The sky: a two-stop gradient, a soft sun (or moon and stars).
        float t = clamp(q0.y / H, 0.0, 1.0);
        vec3 col = mix(s.color2.rgb, s.color.rgb, smoothstep(0.0, 0.85, t));
        if (scene != 4 && scene != 5) {
            vec2 sunAt = vec2(s.size.x * 0.76, H * 0.2);
            float sun = length(q0 - sunAt);
            vec3 sunCol = mix(vec3(1.0, 0.97, 0.85), vec3(0.92, 0.94, 1.0), night);
            float r = mix(70.0, 52.0, night);
            col = mix(col, sunCol * 1.15, (1.0 - smoothstep(r - 1.5, r + 1.5, sun)));
            col += sunCol * 0.22 * exp(-max(sun - r, 0.0) / 140.0) * (1.0 - 0.5 * night);
            if (night > 0.0) {
                // A crescent: the moon's shadow side.
                float bite = length(q0 - sunAt - vec2(22.0, -14.0)) - r * 0.92;
                col = mix(col, mix(s.color2.rgb, s.color.rgb, t), (1.0 - smoothstep(-1.5, 1.5, bite)) * (1.0 - smoothstep(r - 1.5, r + 1.5, sun)) * night * 0.85);
                // Stars that twinkle.
                vec2 cell = floor(q0 / 46.0);
                float hs = hash12(uvec2(uvec2(cell + 1000.0)));
                vec2 starAt = (cell + 0.25 + 0.5 * vec2(hash12(uvec2(cell + 2000.0)), hash12(uvec2(cell + 3000.0)))) * 46.0;
                float tw = 0.6 + 0.4 * sin(time * (1.5 + hs * 2.0) + hs * 30.0);
                float star = sdStar(q0 - starAt, 4, 3.2 + hs * 2.0, 0.9) ;
                col = mix(col, vec3(1.0, 0.98, 0.9), step(0.78, hs) * (1.0 - smoothstep(-0.8, 0.8, star)) * night * tw * (1.0 - t));
            }
        } else {
            // Caves: faint crystal glimmers in the dark.
            vec2 cell = floor(q / 90.0);
            float hs = hash12(uvec2(uvec2(cell + 500.0)));
            vec2 at = (cell + 0.5) * 90.0 + (vec2(hs, fract(hs * 7.0)) - 0.5) * 50.0;
            float g = exp(-length(q - at) / 6.0) * step(0.7, hs) * (0.6 + 0.4 * sin(time * 2.0 + hs * 20.0));
            col += s.color.rgb * g * 0.8;
        }
        outColor = vec4(col, 1.0);
        return;
    }

    if (layer == 1) {
        // Clouds drifting slowly; in caves, hanging stalactites far back.
        vec4 acc = vec4(0.0);
        if (scene == 4 || scene == 5) {
            float x = q.x;
            float cell = floor(x / 70.0);
            float hs = h1(cell);
            float len = 60.0 + hs * 160.0;
            float cx = (cell + 0.5) * 70.0;
            float d = sdTriangle(q - vec2(0.0, -scroll.y * 0.0), vec2(cx - 22.0 - hs * 10.0, -20.0), vec2(cx + 22.0 + hs * 10.0, -20.0),
                                 vec2(cx + (hs - 0.5) * 10.0, len)) - 6.0;
            outColor = silhouette(d, s.color2.rgb, s.color.rgb, px, 30.0, 0.25);
            return;
        }
        float drift = time * 6.0;
        for (int row = 0; row < 2; ++row) {
            float fr = float(row);
            float span = 760.0 + fr * 260.0;
            float x = q.x + drift * (1.0 + fr * 0.6) + fr * 300.0;
            float cell = floor(x / span);
            float hs = h1(cell + fr * 31.0);
            vec2 at = vec2((cell + 0.5) * span + (hs - 0.5) * span * 0.4, 150.0 + fr * 150.0 + hs * 80.0 - scroll.y * 0.0);
            float size = 120.0 + hs * 90.0 - fr * 25.0;
            float d = cloudShape(vec2(x, q0.y + scroll.y), at, size, hs * 10.0);
            float present = step(0.25, hs);
            float shade = smoothstep(at.y - size * 0.25, at.y + size * 0.1, q0.y + scroll.y);
            vec3 col = mix(s.color.rgb, s.color2.rgb, shade * 0.8);
            float cover = clamp(0.5 - d / px, 0.0, 1.0) * present;
            acc = over(acc, vec4(col, cover * (0.92 - fr * 0.15)));
        }
        outColor = acc;
        return;
    }

    if (layer == 2 || layer == 3) {
        bool near = layer == 3;
        vec3 body = s.color.rgb;
        vec3 rim = s.color2.rgb;
        float base = near ? H * 0.78 : H * 0.62;
        float d = 1e5;
        if (scene == 0) {
            // Rolling green hills, round trees on the near ones; far: a soft
            // distant isle and the sea line.
            float y = ridge(q.x, base, near ? 70.0 : 55.0, near ? 0.0042 : 0.0026, near ? 1.3 : 4.2);
            d = y - q.y;
            if (near) {
                float cell = floor(q.x / 260.0);
                float hs = h1(cell + 7.0);
                float tx = (cell + 0.5) * 260.0 + (hs - 0.5) * 120.0;
                float ty = ridge(tx, base, 70.0, 0.0042, 1.3);
                d = min(d, treeShape(q, tx, ty + 6.0, 110.0 + hs * 50.0, hs * 9.0) + step(hs, 0.35) * 1e4);
            } else {
                float cell = floor(q.x / 180.0);
                float hs = h1(cell + 3.0);
                float tx = (cell + 0.5) * 180.0;
                float ty = ridge(tx, base, 55.0, 0.0026, 4.2);
                d = min(d, treeShape(q, tx, ty + 4.0, 60.0 + hs * 30.0, hs * 5.0) + step(hs, 0.5) * 1e4);
            }
        } else if (scene == 1) {
            if (!near) {
                // The far sea: the horizon band, sparkles and slow wave
                // lines, and distant islands sitting on the horizon.
                float horizon = H * 0.6;
                float sea = q.y - horizon;
                vec4 c = vec4(0.0);
                vec3 seaCol = mix(rim, body, smoothstep(0.0, H * 0.4, sea));
                float sparkle = step(0.985, hash12(uvec2(uvec2(floor(vec2(q.x / 9.0, q0.y / 5.0)) + 4000.0))))
                                * (0.5 + 0.5 * sin(time * 3.0 + q.x));
                seaCol = mix(seaCol, vec3(1.0), sparkle * 0.7 * (1.0 - night * 0.5));
                float wave = abs(fract(sea / 22.0 + sin(q.x * 0.01 + time * 0.6) * 0.2) - 0.5);
                seaCol = mix(seaCol, rim, smoothstep(0.08, 0.0, wave) * 0.35 * smoothstep(0.0, 40.0, sea));
                c = over(vec4(seaCol, clamp(0.5 + sea, 0.0, 1.0)), c);
                // Islands: a soft bump every 1600 units, cut at the horizon.
                float local = mod(q.x, 1600.0) - 800.0;
                float isleTop = horizon - 70.0 * (1.0 - smoothstep(0.0, 260.0, abs(local))) - 18.0 * sin(q.x * 0.03);
                float isle = max(isleTop - q.y, q.y - horizon - 1.0);
                vec4 l = silhouette(isle, mix(body, vec3(0.42, 0.62, 0.52), 0.55), rim, px, 16.0, 0.25);
                c = over(l, c);
                outColor = c;
                return;
            }
            // Near: harbor hills with houses' rooftops and the cliffs.
            float y = ridge(q.x, base, 50.0, 0.0035, 0.7);
            d = y - q.y;
            float cell = floor(q.x / 150.0);
            float hs = h1(cell + 11.0);
            float hx = (cell + 0.5) * 150.0;
            float hy = ridge(hx, base, 50.0, 0.0035, 0.7);
            float house = sdBox(q - vec2(hx, hy - 30.0), vec2(34.0 + hs * 10.0, 32.0));
            float roof = sdTriangle(q, vec2(hx - 50.0, hy - 56.0), vec2(hx + 50.0, hy - 56.0), vec2(hx, hy - 100.0 - hs * 20.0));
            d = min(d, min(house, roof) - 4.0 + step(hs, 0.3) * 1e4);
        } else if (scene == 2 || scene == 5) {
            // Giant trees: tall trunks and big canopies.
            float span = near ? 520.0 : 340.0;
            float cell = floor(q.x / span);
            float hs = h1(cell + (near ? 21.0 : 5.0));
            float tx = (cell + 0.5) * span + (hs - 0.5) * span * 0.3;
            float w = (near ? 70.0 : 40.0) * (0.8 + hs * 0.5);
            float trunk = sdBox(q - vec2(tx, H * 0.7), vec2(w * (1.0 + smoothstep(H * 0.5, H * 1.0, q.y) * 0.4), H * 0.7));
            float canopy = 1e5;
            for (int i = 0; i < 4; ++i) {
                float fi = float(i);
                canopy = min(canopy, length(q - vec2(tx + (fi - 1.5) * w * 1.3, (near ? 120.0 : 200.0) + sin(fi * 2.0 + hs * 9.0) * 40.0))
                                         - w * (1.5 + 0.3 * h1(fi + hs)));
            }
            float floorLine = ridge(q.x, base + (near ? 40.0 : 20.0), 30.0, 0.004, 3.0) - q.y;
            d = min(min(trunk, canopy), floorLine);
        } else if (scene == 3) {
            // Peaks with snow caps, pine hills nearer.
            if (!near) {
                float x = q.x * 0.0021;
                float y = H * 0.58 - 230.0 * abs(sin(x * 3.14)) * (0.6 + 0.4 * sin(x * 1.3 + 1.0)) - 60.0 * abs(sin(x * 9.0));
                d = y - q.y;
                vec4 c = silhouette(d, body, rim, px, 30.0, 0.3);
                // The snow line.
                float snow = q.y - (y + 70.0 + 20.0 * sin(q.x * 0.03));
                c.rgb = mix(c.rgb, mix(vec3(1.0), rim, 0.3), (1.0 - smoothstep(-1.0, 1.0, snow)) * c.a);
                outColor = c;
                return;
            }
            float y = ridge(q.x, base, 60.0, 0.003, 2.5);
            d = y - q.y;
            float cell = floor(q.x / 70.0);
            float hs = h1(cell + 13.0);
            float tx = (cell + 0.5) * 70.0;
            d = min(d, pineShape(q, tx, ridge(tx, base, 60.0, 0.003, 2.5) + 10.0, 90.0 + hs * 60.0) + step(hs, 0.3) * 1e4);
        } else {
            // Caves: rock columns and ledges, glowing crystals.
            float span = near ? 380.0 : 240.0;
            float cell = floor(q.x / span);
            float hs = h1(cell + (near ? 41.0 : 17.0));
            float cx = (cell + 0.5) * span + (hs - 0.5) * span * 0.4;
            float w = (near ? 60.0 : 36.0) * (0.7 + hs * 0.6);
            float column = sdBox(q - vec2(cx, H * 0.5), vec2(w + 10.0 * sin(q.y * 0.02 + hs * 6.0), H));
            float floorLine = ridge(q.x, base + 60.0, 40.0, 0.004, 1.0) - q.y;
            d = min(column + step(hs, 0.3) * 1e4, floorLine);
            vec4 c = silhouette(d, body, rim, px, 40.0, 0.25);
            // Crystals glowing on the rocks.
            float ccell = floor(q.x / 110.0);
            float ch = h1(ccell + 77.0);
            vec2 at = vec2((ccell + 0.5) * 110.0, ridge((ccell + 0.5) * 110.0, base + 60.0, 40.0, 0.004, 1.0) - 6.0);
            float crystal = sdTriangle(q, at + vec2(-10.0, 0.0), at + vec2(10.0, 0.0), at + vec2(0.0, -34.0 - ch * 20.0));
            float glow = (1.0 - smoothstep(-1.0, 1.0, crystal)) * step(0.5, ch);
            c = over(vec4(s.color2.rgb * 1.3 + vec3(0.2, 0.1, 0.4), glow), c);
            c.rgb += vec3(0.5, 0.4, 1.0) * exp(-max(crystal, 0.0) / 18.0) * step(0.5, ch) * 0.35 * (0.7 + 0.3 * sin(time * 2.0 + ch * 9.0));
            outColor = c;
            return;
        }
        outColor = silhouette(d, body, rim, px, near ? 26.0 : 20.0, near ? 0.38 : 0.25);
        return;
    }

    // Layer 4: light - sun rays by day, a mist band, floating glow in caves.
    vec4 c = vec4(0.0);
    if (scene == 4 || scene == 5) {
        float mist = smoothstep(H * 0.55, H * 0.95, q0.y) * (0.5 + 0.5 * sin(q.x * 0.004 + time * 0.2));
        c = vec4(s.color.rgb, mist * 0.12);
    } else {
        float rays = 0.0;
        for (int i = 0; i < 4; ++i) {
            float fi = float(i);
            float x = q0.x - s.size.x * (0.55 + fi * 0.1) - (q0.y) * 0.45;
            rays += exp(-abs(x + sin(time * 0.15 + fi) * 30.0) / (26.0 + fi * 10.0)) * (0.5 + 0.5 * sin(fi * 2.3 + time * 0.3));
        }
        rays *= (1.0 - smoothstep(0.0, H * 0.8, q0.y)) * (1.0 - night) * 0.08;
        float mist = smoothstep(H * 0.6, H, q0.y) * 0.08;
        c = vec4(s.color.rgb, clamp(rays + mist, 0.0, 0.3));
    }
    outColor = c;
}
