#version 450
#include "gamelib/sprite.glsl"
#include "gamelib/anime_face.glsl"

// Realm's chibi avatar: the hero, every townsperson and the job outfits are
// ONE construction - a round head about half the height with big anime eyes,
// a small rounded body, short limbs with mitten hands - dressed in equipment
// layers that the game swaps (hair, hat, top, bottom, shoes, gloves, weapon,
// cape). Built on GameLib's anime library (ink lines, one-tone cel shading,
// the face kit).
//
// Construction: one skeleton of anchors (Rig) set per pose in rigFor(); every
// layer hangs on those anchors in a fixed order: cape, back hair, far arm,
// far leg, near leg, bottom, top, neck and head, face, front hair, hat,
// weapon, near arm. The body is turned a little towards +x like the face
// (three-quarter view): the far side narrower, seams off-center.
//
// Design space: 128 x 128 units over the quad height, feet on y = +56,
// facing +x (params.x mirrors). Fields (see src/app/anime_look.hpp):
//   color   hair color            color2  top color
//   params  x facing, y pose (0 idle, 1 walk, 2 jump, 3 swing, 4 hurt,
//           5 cheer, 6 climb (from behind), 7 cast, 8 prone, 9 throw,
//           10 sit), z phase 0..1, w expression blend A -> B
//   bits.x  0-2 hair, 3-5 hat, 6-8 top, 9-10 bottom, 11-12 shoes,
//           13-14 gloves, 15-17 weapon, 18-19 cape, 20-22 eye color,
//           23-24 skin, 25-28 expression A, 29 skeleton overlay,
//           30 a glow outline (a buff), 31 ghost (an afterimage: flat tint)
//   bits.y  0-3 expression B, 4-7 hat color, 8-11 bottom color,
//           12-15 shoe color, 16-19 cape color, 20-23 accent - palette
//           indices (kAnimePalette) - 24-31 seed (blinks, breathing)

layout(location = 0) in vec2 inUv;
layout(location = 0) out vec4 outColor;

AnimeCanvas C;
vec2 P;

uint gWeapon, gTop, gGloves;
vec3 gAccent, gTopColor;
AnimeMaterial gSleeve, gForearm, gHand, gTrim, gSteel, gWood, gLeather, gGem;

const uint kEyes[8] = uint[8](0x3A7CE8u, 0x2BA86Au, 0xD8892Cu, 0x9058E0u, 0xD8384Au, 0x18A8B8u, 0x7A4A34u, 0xC0408Au);
const uint kSkins[4] = uint[4](0xFFE9DCu, 0xFAD6BCu, 0xE0AE88u, 0xA87452u);

// Equipment kinds (the game's ItemInfo::style picks them).
const uint kHairSprout = 0u, kHairBob = 1u, kHairPonytail = 2u, kHairLong = 3u, kHairBuns = 4u, kHairSpiky = 5u,
           kHairBraid = 6u, kHairCurly = 7u;
const uint kHatNone = 0u, kHatBeret = 1u, kHatWizard = 2u, kHatBandana = 3u, kHatHelmet = 4u, kHatFeather = 5u,
           kHatFlower = 6u, kHatHood = 7u;
const uint kTopTee = 0u, kTopTunic = 1u, kTopRobe = 2u, kTopPlate = 3u, kTopCoat = 4u, kTopApron = 5u, kTopVest = 6u,
           kTopSailor = 7u;
const uint kBottomPants = 0u, kBottomShorts = 1u, kBottomSkirt = 2u, kBottomGreaves = 3u;
const uint kShoesBoots = 0u, kShoesShoes = 1u, kShoesSabatons = 2u, kShoesSandals = 3u;
const uint kGlovesNone = 0u, kGlovesCloth = 1u, kGlovesGauntlets = 2u;
const uint kWeaponNone = 0u, kWeaponBranch = 1u, kWeaponSword = 2u, kWeaponAxe = 3u, kWeaponStaff = 4u, kWeaponWand = 5u,
           kWeaponBroadsword = 6u, kWeaponTool = 7u;
const uint kCapeNone = 0u, kCapeCape = 1u, kCapeScarf = 2u, kCapeMantle = 3u;

const int kPoseIdle = 0, kPoseWalk = 1, kPoseJump = 2, kPoseSwing = 3, kPoseHurt = 4, kPoseCheer = 5, kPoseClimb = 6,
          kPoseCast = 7, kPoseProne = 8, kPoseThrow = 9, kPoseSit = 10;

const float kUpperArm = 7.5, kForearm = 7.0, kThigh = 7.0, kShin = 7.0;
const vec3 kFar = vec3(0.82, 0.82, 0.92);

struct Rig {
    vec2 head;
    vec2 neck;
    vec2 shoulderN;
    vec2 shoulderF;
    vec2 elbowN;
    vec2 elbowF;
    vec2 handN;
    vec2 handF;
    vec2 hipN;
    vec2 hipF;
    vec2 kneeN;
    vec2 kneeF;
    vec2 footN;
    vec2 footF;
    vec2 grip;
    vec2 gripDir;
    float lean;
    float headTilt;
};

vec2 ik(vec2 root, vec2 target, float a, float b, float bend)
{
    vec2 d = target - root;
    float len = clamp(length(d), 1e-3, a + b - 1e-3);
    vec2 dir = d / max(length(d), 1e-3);
    float x = (a * a - b * b + len * len) / (2.0 * len);
    float y = sqrt(max(a * a - x * x, 0.0));
    return root + dir * x + vec2(-dir.y, dir.x) * y * bend;
}

vec2 reach(vec2 root, vec2 target, float maxLength)
{
    vec2 d = target - root;
    float l = length(d);
    return l > maxLength ? root + d / l * maxLength : target;
}

bool twoHanded(uint weapon)
{
    return weapon == kWeaponBroadsword || weapon == kWeaponAxe;
}

Rig rigFor(int pose, float ph, float time, float seed, uint weapon)
{
    float tau = 2.0 * kAnimePi;
    float wave = sin(ph * tau);
    float lift = cos(ph * tau);
    float breathe = sin(time * 2.4 + seed * 6.0);
    float bob = pose == kPoseIdle ? breathe * 0.6 : 0.0;
    if (pose == kPoseWalk) {
        bob = -abs(sin(ph * tau)) * 2.6 + 1.0;
    } else if (pose == kPoseJump) {
        bob = -1.5;
    } else if (pose == kPoseSwing) {
        bob = ph < 0.35 ? -ph * 3.0 : -1.05 + (ph - 0.35) * 3.4;
    } else if (pose == kPoseCheer) {
        bob = -abs(sin(time * 7.0)) * 3.0;
    } else if (pose == kPoseCast || pose == kPoseThrow) {
        bob = sin(ph * kAnimePi) * -1.2;
    }
    Rig r;
    r.lean = 0.0;
    r.headTilt = 0.0;
    r.head = vec2(1.5, -7.5 + bob);
    r.neck = vec2(1.5, 16.0 + bob);
    r.shoulderN = vec2(9.0, 20.0 + bob);
    r.shoulderF = vec2(-6.0, 20.0 + bob);
    r.hipN = vec2(5.5, 36.0 + bob * 0.6);
    r.hipF = vec2(-2.5, 36.0 + bob * 0.6);
    r.footN = vec2(7.5, 56.0);
    r.footF = vec2(-3.5, 56.0);
    r.handN = vec2(14.0, 31.0 + bob);
    r.handF = vec2(-9.5, 30.5 + bob);
    float bendN = 1.0;
    float bendF = 1.0;

    if (pose == kPoseIdle) {
        r.handN += vec2(0.0, breathe * 0.5);
        r.handF += vec2(0.0, -breathe * 0.4);
        r.headTilt = 0.0;
    } else if (pose == kPoseWalk) {
        r.footN = vec2(7.5 + 8.5 * wave, 56.0 - max(0.0, lift) * 5.0);
        r.footF = vec2(-3.5 - 8.5 * wave, 56.0 - max(0.0, -lift) * 5.0);
        r.handN = vec2(14.0 - 8.0 * wave, 31.0 + bob - max(0.0, -wave) * 3.0);
        r.handF = vec2(-10.5 + 8.0 * wave, 31.0 + bob - max(0.0, wave) * 3.0);
        r.lean = 0.05;
    } else if (pose == kPoseJump) {
        float up = clamp(ph, 0.0, 1.0); // 0 rising, 1 falling
        r.footN = mix(vec2(10.0, 49.0), vec2(9.0, 53.0), up);
        r.footF = mix(vec2(-5.0, 52.5), vec2(-6.0, 51.0), up);
        r.handN = mix(vec2(19.0, 9.0), vec2(21.0, 17.0), up);
        r.handF = mix(vec2(-15.0, 8.0), vec2(-17.0, 15.0), up);
        bendN = -1.0;
        bendF = 1.0;
    } else if (pose == kPoseSwing) {
        r.footN = vec2(13.0, 56.0);
        r.footF = vec2(-7.0, 56.0);
        // Wind-up (0..0.35): the hand goes up behind the head; swing
        // (0.35..0.55): over and down in front; then the follow-through.
        vec2 up = vec2(6.0, 0.0);
        vec2 front = vec2(23.0, 31.0);
        r.handN = (ph < 0.35 ? mix(vec2(14.0, 31.0), up, smoothstep(0.0, 0.35, ph))
                             : mix(up, front, smoothstep(0.35, 0.55, ph))) + vec2(0.0, bob);
        bendN = ph < 0.42 ? -1.0 : 1.0;
        if (twoHanded(weapon)) {
            r.handF = r.handN + vec2(-4.0, 2.5);
            bendF = bendN;
        } else {
            r.handF = vec2(-12.0, 27.0 + bob);
        }
        r.lean = ph < 0.35 ? -0.1 * smoothstep(0.0, 0.35, ph) : 0.14 * (1.0 - smoothstep(0.6, 1.0, ph));
    } else if (pose == kPoseHurt) {
        r.handN = vec2(19.0, 13.0);
        r.handF = vec2(-15.0, 11.0);
        r.footN = vec2(10.0, 55.0);
        r.footF = vec2(-6.0, 56.0);
        bendN = -1.0;
        r.headTilt = -0.12;
    } else if (pose == kPoseCheer) {
        float pump = sin(time * 9.0);
        r.handN = vec2(18.0, -1.0 + pump * 1.5 + bob);
        r.handF = vec2(-14.0, 1.0 - pump * 1.5 + bob);
        bendN = -1.0;
        bendF = 1.0;
        r.headTilt = 0.06 * pump;
    } else if (pose == kPoseClimb) {
        r.head = vec2(0.0, -9.0);
        r.shoulderN = vec2(10.0, 18.0);
        r.shoulderF = vec2(-10.0, 18.0);
        r.hipN = vec2(4.5, 37.0);
        r.hipF = vec2(-4.5, 37.0);
        r.handN = vec2(3.5, -3.0 - wave * 3.0);
        r.handF = vec2(-3.5, -3.0 + wave * 3.0);
        r.footN = vec2(5.0, 56.0 - max(0.0, -wave) * 6.0);
        r.footF = vec2(-5.0, 56.0 - max(0.0, wave) * 6.0);
        bendN = 1.0;
        bendF = -1.0;
    } else if (pose == kPoseCast) {
        // Both hands forward, the weapon raised; a little rise on the toes.
        float push = smoothstep(0.0, 0.3, ph) * (1.0 - smoothstep(0.75, 1.0, ph));
        r.handN = mix(vec2(15.0, 30.0), vec2(24.0, 16.0), push) + vec2(0.0, bob);
        r.handF = mix(vec2(-11.0, 30.0), vec2(14.0, 20.0), push) + vec2(0.0, bob);
        r.footN = vec2(11.0, 56.0);
        r.footF = vec2(-6.0, 56.0);
        bendN = -1.0;
        bendF = -1.0;
        r.lean = 0.08 * push;
    } else if (pose == kPoseThrow) {
        float wind = smoothstep(0.0, 0.4, ph);
        float fling = smoothstep(0.4, 0.6, ph);
        r.handN = mix(mix(vec2(14.0, 30.0), vec2(-6.0, 6.0), wind), vec2(25.0, 20.0), fling) + vec2(0.0, bob);
        r.handF = vec2(14.0 - fling * 18.0, 24.0 + bob);
        r.footN = vec2(12.0, 56.0);
        r.footF = vec2(-6.0, 56.0);
        bendN = -1.0;
        r.lean = mix(-0.08 * wind, 0.12, fling);
    } else if (pose == kPoseProne) {
        // Lying flat, chin on the hands (MapleStory's prone).
        // A level lie: the body flat at a low height, the head at the front
        // resting on the folded arms, the legs straight behind.
        r.head = vec2(19.0, 33.5);
        r.neck = vec2(9.0, 49.5);
        r.shoulderN = vec2(8.0, 49.0);
        r.shoulderF = vec2(6.0, 48.0);
        r.hipN = vec2(-12.0, 50.0);
        r.hipF = vec2(-13.0, 49.0);
        r.handN = vec2(21.0, 52.5);
        r.handF = vec2(17.0, 52.0);
        r.footN = vec2(-29.0, 54.0);
        r.footF = vec2(-30.0, 53.0);
        bendN = 1.0;
        bendF = 1.0;
    } else if (pose == kPoseSit) {
        r.hipN = vec2(4.0, 47.0);
        r.hipF = vec2(-4.0, 47.0);
        r.head.y += 10.0;
        r.neck.y += 10.0;
        r.shoulderN.y += 10.0;
        r.shoulderF.y += 10.0;
        r.footN = vec2(18.0, 56.0);
        r.footF = vec2(9.0, 56.0);
        r.handN = vec2(14.0, 45.0);
        r.handF = vec2(-9.0, 44.0);
    }
    if ((weapon == kWeaponStaff || weapon == kWeaponTool) && (pose == kPoseIdle || pose == kPoseWalk)) {
        r.handN = vec2(18.0, 27.0 + bob - (pose == kPoseWalk ? wave * 1.2 : 0.0));
    }
    if (twoHanded(weapon) && (pose == kPoseIdle || pose == kPoseWalk)) {
        r.handN = vec2(13.0, 28.0 + bob);
    }
    float arm = kUpperArm + kForearm - 0.1;
    if (pose != kPoseClimb && pose != kPoseProne) {
        r.handN = reach(r.shoulderN, r.handN, arm);
        r.handF = reach(r.shoulderF, r.handF, arm);
    }
    r.elbowN = ik(r.shoulderN, r.handN, kUpperArm, kForearm, bendN);
    r.elbowF = ik(r.shoulderF, r.handF, kUpperArm, kForearm, bendF);
    vec2 ankleN = r.footN - vec2(1.0, 3.5);
    vec2 ankleF = r.footF - vec2(1.0, 3.5);
    float kneeBend = pose == kPoseProne ? 1.0 : -1.0;
    r.kneeN = ik(r.hipN, ankleN, kThigh, kShin, kneeBend);
    r.kneeF = ik(r.hipF, ankleF, kThigh, kShin, kneeBend);

    r.grip = r.handN;
    vec2 forearm = normalize(r.handN - r.elbowN);
    if (weapon == kWeaponStaff || weapon == kWeaponTool) {
        r.gripDir = pose == kPoseCast ? normalize(vec2(0.45, -1.0)) : pose == kPoseProne ? normalize(vec2(1.0, -0.12)) : (pose == kPoseSwing ? forearm : vec2(0.04, -1.0));
    } else if (weapon == kWeaponWand) {
        r.gripDir = pose == kPoseCast ? normalize(vec2(1.0, -0.5)) : forearm;
    } else if (pose == kPoseSwing) {
        float a = mix(-1.75, 0.85, smoothstep(0.35, 0.55, ph));
        r.gripDir = vec2(cos(a), sin(a));
    } else if (pose == kPoseProne) {
        r.gripDir = normalize(vec2(1.0, -0.08)); // held forward along the ground
    } else if (pose == kPoseJump || pose == kPoseHurt || pose == kPoseThrow) {
        r.gripDir = forearm;
    } else if (pose == kPoseCheer || twoHanded(weapon)) {
        r.gripDir = normalize(vec2(0.3, -1.0));
    } else if (pose == kPoseCast) {
        r.gripDir = normalize(vec2(0.8, -0.6));
    } else {
        r.gripDir = normalize(vec2(0.7, 0.72));
    }
    return r;
}

// ---------------------------------------------------------------------------
// Head and hair

float faceShape(vec2 p, vec2 head, float turn)
{
    // A wide soft dome with full cheeks and a small rounded chin, turned.
    float dome = sdEllipse(p - head - vec2(0.0, -2.5), vec2(21.5, 19.5));
    float cheeks = sdEllipse(p - head - vec2(turn * 4.5, 7.5), vec2(15.5, 10.5));
    return animeSmoothUnion(dome, cheeks, 6.0);
}

// The hair cap over the crown, cut at the hairline (it shows the forehead).
float capShape(vec2 p, vec2 head, uint style)
{
    float r = style == kHairCurly ? 25.5 : 24.0;
    float cap = sdEllipse(p - head - vec2(-0.5, -5.5), vec2(r, r - 1.5));
    float hairline = p.y - head.y + 10.0 - (p.x - head.x) * 0.1;
    return max(cap, hairline);
}

// Bangs over the forehead (soft, rounded locks that end above the eyes).
float fringeShape(vec2 p, vec2 hc, uint style)
{
    float d;
    if (style == kHairSpiky) {
        d = animeHairLock(p, hc + vec2(-14.0, -20.0), hc + vec2(-19.0, -10.0), hc + vec2(-19.0, 1.0), 6.5);
        d = min(d, animeHairLock(p, hc + vec2(-6.0, -23.0), hc + vec2(-9.0, -12.0), hc + vec2(-8.0, -1.0), 6.5));
        d = min(d, animeHairLock(p, hc + vec2(3.0, -23.0), hc + vec2(5.0, -12.0), hc + vec2(3.5, -2.0), 6.5));
        d = min(d, animeHairLock(p, hc + vec2(11.0, -21.0), hc + vec2(15.0, -10.0), hc + vec2(13.0, 0.0), 6.0));
        d = min(d, animeHairLock(p, hc + vec2(17.0, -15.0), hc + vec2(23.0, -4.0), hc + vec2(21.0, 7.0), 5.5));
        return d;
    }
    if (style == kHairBob || style == kHairLong || style == kHairBraid) {
        // Straight bangs: five soft locks side by side with rounded points,
        // the outer ones framing the cheeks.
        d = animeHairLock(p, hc + vec2(-11.0, -20.0), hc + vec2(-12.5, -12.0), hc + vec2(-11.5, -5.0), 6.0);
        d = min(d, animeHairLock(p, hc + vec2(-3.5, -21.0), hc + vec2(-4.0, -13.0), hc + vec2(-3.0, -4.5), 6.0));
        d = min(d, animeHairLock(p, hc + vec2(4.5, -21.0), hc + vec2(5.0, -13.0), hc + vec2(4.5, -5.0), 6.0));
        d = min(d, animeHairLock(p, hc + vec2(11.5, -19.0), hc + vec2(13.0, -12.0), hc + vec2(12.0, -6.0), 5.5));
        d = min(d, sdRoundBox(p - hc - vec2(0.5, -17.0), vec2(17.0, 4.0), 4.0));
        d = min(d, animeHairLock(p, hc + vec2(-16.0, -12.0), hc + vec2(-21.0, -1.0), hc + vec2(-18.5, 11.0), 5.5));
        d = min(d, animeHairLock(p, hc + vec2(17.0, -12.0), hc + vec2(22.0, 0.0), hc + vec2(19.0, 11.0), 5.0));
        return d;
    }
    if (style == kHairCurly) {
        d = sdCircle(p - hc - vec2(-12.0, -14.0), 7.5);
        d = min(d, sdCircle(p - hc - vec2(-2.0, -17.0), 7.5));
        d = min(d, sdCircle(p - hc - vec2(8.5, -16.0), 7.0));
        d = min(d, sdCircle(p - hc - vec2(17.0, -10.0), 6.5));
        d = min(d, sdCircle(p - hc - vec2(-19.0, -5.0), 6.0));
        return d;
    }
    // Sprout, ponytail, buns: a soft side-swept fringe of four locks.
    d = animeHairLock(p, hc + vec2(-12.0, -19.0), hc + vec2(-18.0, -9.0), hc + vec2(-17.0, 2.0), 7.0);
    d = min(d, animeHairLock(p, hc + vec2(-3.0, -21.0), hc + vec2(-6.5, -11.0), hc + vec2(-5.0, -2.0), 7.0));
    d = min(d, animeHairLock(p, hc + vec2(6.0, -21.0), hc + vec2(8.0, -11.0), hc + vec2(5.0, -3.0), 6.5));
    d = min(d, animeHairLock(p, hc + vec2(14.0, -17.0), hc + vec2(19.0, -7.0), hc + vec2(17.5, 4.0), 6.0));
    return d;
}

// ---------------------------------------------------------------------------
// Limbs

void drawArm(vec2 shoulder, vec2 elbow, vec2 hand, bool wide, AnimeMaterial sleeve, AnimeMaterial forearm,
             AnimeMaterial handMat, bool gloved)
{
    if (wide) {
        animePart(C, sdLockBelly(P, shoulder, elbow, hand + (hand - elbow) * 0.15, 4.2, 6.0), sleeve);
    } else {
        animePart(C, sdSegment(P, elbow, hand, 2.8), forearm);
        animePart(C, sdSegment(P, shoulder, elbow, 3.5), sleeve);
        if (gTop == kTopTee || gTop == kTopSailor) {
            // A colored cuff, so the sleeve reads against the shirt.
            float cuff = max(sdSegment(P, shoulder, elbow, 3.5), abs(dot(P - mix(shoulder, elbow, 0.82), normalize(elbow - shoulder + vec2(1e-4)))) - 1.3);
            animePart(C, cuff, animeTinted(gTrim, sleeve.base / max(gSleeve.base, vec3(1e-3))));
        }
    }
    // A mitten: a round hand with a little thumb bump.
    vec2 dir = normalize(hand - elbow + vec2(1e-4));
    float mitt = animeSmoothUnion(sdCircle(P - hand - dir * 0.6, 4.1), sdCircle(P - hand - vec2(-dir.y, dir.x) * 2.6, 1.9), 1.5);
    animePart(C, mitt, handMat);
    if (gloved) {
        animePart(C, max(sdCircle(P - mix(elbow, hand, 0.66), 4.0), -sdCircle(P - hand, 3.3)), handMat);
    }
}

void drawLeg(vec2 hip, vec2 knee, vec2 foot, AnimeMaterial thigh, AnimeMaterial shin, AnimeMaterial shoe, uint shoes,
             bool far)
{
    vec2 ankle = foot - vec2(1.0, 3.5);
    animePart(C, sdSegment(P, knee, ankle, 3.6), shin);
    animePart(C, sdSegment(P, hip, knee, 4.2), thigh);
    // Chunky rounded shoes, toe forward (+x).
    float toe = sdRoundBox(P - foot - vec2(2.0, -3.2), vec2(6.2, 3.4), 3.2);
    if (shoes == kShoesBoots || shoes == kShoesSabatons) {
        float shaft = sdRoundBox(P - ankle - vec2(-0.2, -1.6), vec2(4.4, 4.2), 2.0);
        animePart(C, shaft, shoe);
        if (shoes == kShoesBoots) {
            animeLine(C, max(abs(P.y - ankle.y + 4.6), shaft), shoe.ink);
        }
    }
    if (shoes == kShoesSandals) {
        animePart(C, toe, gHand); // bare toes (skin) with a strap
        animePart(C, max(toe + 0.2, abs(P.x - foot.x - 1.5) - 1.4), shoe);
        animePart(C, sdRoundBox(P - foot - vec2(2.0, -0.6), vec2(6.6, 1.0), 1.0), shoe);
    } else {
        animePart(C, toe, shoe);
        if (shoes == kShoesSabatons) {
            animeLine(C, max(abs(P.x - foot.x - 2.5), toe + 0.8), shoe.ink);
        } else if (!far) {
            animeGloss(C, P, toe, foot + vec2(3.5, -4.6), vec2(2.4, 1.0), 0.4);
        }
    }
}

// ---------------------------------------------------------------------------
// Weapons and the near arm (drawn last)

void drawWeaponAndNearArm(Rig r, int pose, float ph, float time)
{
    vec2 g = r.grip;
    vec2 dir = r.gripDir;
    vec2 side = vec2(-dir.y, dir.x);
    bool swingSmear = pose == kPoseSwing && ph > 0.37 && ph < 0.72;
    if (swingSmear && gWeapon != kWeaponNone && gWeapon != kWeaponWand) {
        // The swing smear: a soft crescent the weapon leaves.
        vec2 q = P - r.shoulderN;
        float ang = atan(q.y, q.x);
        float a1 = atan(r.handN.y - r.shoulderN.y, r.handN.x - r.shoulderN.x) + 0.2;
        float a0 = max(a1 - 2.0, -0.95);
        float inArc = step(a0, ang) * step(ang, a1);
        float radius = twoHanded(gWeapon) ? 34.0 : 29.0;
        float smear = (1.0 - smoothstep(0.0, 1.0, abs(length(q) - radius) / 10.0)) * inArc * smoothstep(0.72, 0.45, ph)
                      * smoothstep(a0, a1, ang);
        C.color = over(vec4(mix(vec3(1.0), gAccent, 0.3), smear * 0.8), C.color);
    }
    if (gWeapon == kWeaponSword || gWeapon == kWeaponBroadsword) {
        bool big = gWeapon == kWeaponBroadsword;
        float len = big ? 38.0 : 29.0;
        vec2 tip = g + dir * len;
        vec2 guard = g + dir * 3.8;
        float blade = big ? sdLockBelly(P, guard, mix(guard, tip, 0.5), tip, 4.4, 1.2)
                          : sdLock(P, guard, mix(guard, tip, 0.5), tip, 3.4, 1.0);
        animePartShine(C, blade, gSteel, animeCover(C, abs(sdSegment(P, guard, tip, 0.0) + 1.0) - 0.45) * 0.85, vec3(1.0));
        animeLine(C, max(sdSegment(P, guard + dir * 3.0, tip - dir * 4.0, 0.0), blade + 1.2), gSteel.ink);
        animePart(C, sdSegment(P, guard - side * (big ? 6.5 : 5.0), guard + side * (big ? 6.5 : 5.0), 1.7), gTrim);
        animePart(C, sdCircle(P - guard, 1.8), gGem);
        animePart(C, sdSegment(P, g - dir * 3.2, guard, 1.6), gLeather);
        animePart(C, sdCircle(P - g + dir * 4.0, 1.9), gTrim);
    } else if (gWeapon == kWeaponAxe) {
        vec2 tip = g + dir * 27.0;
        animePart(C, sdSegment(P, g - dir * 6.0, tip, 1.8), gWood);
        vec2 headAt = tip - dir * 4.5;
        float bladeA = sdLockBelly(P, headAt, headAt + side * 6.0 + dir * 1.5, headAt + side * 10.5, 3.2, 7.0);
        float bladeB = sdLock(P, headAt, headAt - side * 4.0, headAt - side * 6.5 + dir * 1.0, 2.5, 1.0);
        animePartShine(C, min(bladeA, bladeB), gSteel, animeCover(C, abs(bladeA + 1.6) - 0.6) * 0.8, vec3(1.0));
    } else if (gWeapon == kWeaponBranch) {
        // A beginner's stick: a knobby branch with a leaf.
        vec2 tip = g + dir * 24.0;
        animePart(C, sdLock(P, g - dir * 3.0, mix(g, tip, 0.5) + side * 1.5, tip, 2.0, 2.8), gWood);
        vec2 leafAt = mix(g, tip, 0.7) + side * 2.0;
        animePart(C, sdLock(P, leafAt, leafAt + side * 3.0 + dir * 1.0, leafAt + side * 6.0 - dir * 1.5, 1.8, 0.3),
                  animeCloth(animeHex(0x7CCB58u)));
    } else if (gWeapon == kWeaponStaff || gWeapon == kWeaponTool) {
        // Upright beside the body; the head above head height.
        float above = clamp((g.y + 36.0) / max(-dir.y, 0.25), 14.0, 52.0);
        vec2 tipAt = g + dir * above;
        vec2 butt = g - dir * (70.0 - above);
        animePart(C, sdSegment(P, butt, tipAt, 1.9), gWood);
        if (gWeapon == kWeaponTool) {
            // A broom or a fishing rod: a townsperson's tool (a straw head).
            float straw = sdTrapezoid(rotate2d(P - butt - dir * 4.0, -atan(dir.x, -dir.y)), 6.5, 2.5, 6.0) - 0.8;
            animePart(C, straw, animeMatte(animeHex(0xE8C870u)));
        } else {
            vec2 orbAt = tipAt + dir * 5.0;
            // A crescent crook holding the gem.
            float crook = max(sdCircle(P - orbAt, 7.8), -sdCircle(P - orbAt - side * 2.0 + dir * 1.5, 6.4));
            animePart(C, crook, gWood);
            float orb = sdCircle(P - orbAt, 4.6);
            float charge = pose == kPoseCast ? smoothstep(0.0, 0.3, ph) * (1.0 - smoothstep(0.7, 1.0, ph)) : 0.35;
            C.color = over(vec4(gAccent * 1.4, glowFalloff(orb, 5.0 + 10.0 * charge) * (0.35 + 0.55 * charge)), C.color);
            animePart(C, orb, animeGlow(mix(gAccent, vec3(1.0), 0.35)));
            animeFlat(C, sdCircle(P - orbAt - vec2(-1.4, -1.6), 1.3), vec3(1.0));
        }
    } else if (gWeapon == kWeaponWand) {
        vec2 tip = g + dir * 17.0;
        animePart(C, sdSegment(P, g - dir * 3.0, tip, 1.3), gWood);
        float star = sdStar(rotate2d(P - tip - dir * 2.5, time * 1.5), 5, 4.6, 2.2) - 0.4;
        C.color = over(vec4(gAccent * 1.5, glowFalloff(star, 6.0) * 0.5), C.color);
        animePart(C, star, animeGlow(mix(gAccent, vec3(1.0), 0.4)));
    }
    drawArm(r.shoulderN, r.elbowN, r.handN, gTop == kTopRobe, gSleeve, gForearm, gHand, gGloves != kGlovesNone);
    if (gTop == kTopPlate) {
        float pad = max(sdEllipse(P - r.shoulderN - vec2(-0.5, -0.5), vec2(6.4, 5.0)), P.y - r.shoulderN.y - 2.5);
        animePartShine(C, pad, animeMetal(gTopColor),
                       animeCover(C, abs(sdEllipse(P - r.shoulderN - vec2(-1.5, -2.0), vec2(3.6, 2.2))) - 0.4) * 0.8,
                       vec3(1.0));
    }
}

void main()
{
    SpriteData s = sprite;
    float k = s.size.y / 128.0;
    float facing = s.params.x < 0.0 ? -1.0 : 1.0;
    vec2 local = spriteLocal(inUv);
    vec2 p = vec2(local.x * facing, local.y) / k;
    int pose = int(s.params.y + 0.5);
    float ph = fract(s.params.z);
    if (pose == kPoseJump) {
        ph = clamp(s.params.z, 0.0, 1.0);
    }
    float blend = clamp(s.params.w, 0.0, 1.0);
    uint bx = s.bits.x;
    uint by = s.bits.y;
    uint hairStyle = bx & 7u;
    uint hat = (bx >> 3) & 7u;
    uint top = (bx >> 6) & 7u;
    uint bottom = (bx >> 9) & 3u;
    uint shoes = (bx >> 11) & 3u;
    uint gloves = (bx >> 13) & 3u;
    uint weapon = (bx >> 15) & 7u;
    uint cape = (bx >> 18) & 3u;
    vec3 iris = animeHex(kEyes[(bx >> 20) & 7u]);
    vec3 skinColor = animeHex(kSkins[(bx >> 23) & 3u]);
    int exprA = int((bx >> 25) & 15u);
    bool debugRig = ((bx >> 29) & 1u) != 0u;
    bool glow = ((bx >> 30) & 1u) != 0u;
    bool ghost = ((bx >> 31) & 1u) != 0u;
    int exprB = int(by & 15u);
    vec3 hatColor = animePalette((by >> 4) & 15u);
    vec3 bottomColor = animePalette((by >> 8) & 15u);
    vec3 shoeColor = animePalette((by >> 12) & 15u);
    vec3 capeColor = animePalette((by >> 16) & 15u);
    vec3 accent = animePalette((by >> 20) & 15u);
    float seed = float((by >> 24) & 0xffu) / 255.0;
    float time = s.time;
    bool climbing = pose == kPoseClimb;
    bool prone = pose == kPoseProne;
    float turn = climbing ? 0.0 : (pose == kPoseCheer ? 0.2 : 0.4);
    float wave = sin(ph * 2.0 * kAnimePi);

    if (pose == kPoseHurt) {
        p = rotate2d(p - vec2(0.0, 56.0), 0.18 * (1.0 - ph)) + vec2(-3.0 * (1.0 - ph), 56.0);
    }
    float breathe = pose == kPoseIdle ? sin(time * 2.4 + seed * 6.0) : 0.0;
    p = animeSquash(p, vec2(0.0, 56.0), 1.0 + breathe * 0.014);
    P = p;

    Rig r = rigFor(pose, ph, time, seed, weapon);
    if (r.lean != 0.0) {
        vec2 pivot = vec2(1.5, 37.0);
        r.head = rotate2d(r.head - pivot, r.lean) + pivot;
        r.neck = rotate2d(r.neck - pivot, r.lean) + pivot;
    }

    C = animeBegin(p, 3.0);
    C.shadeDepth = 4.2;
    AnimeMaterial skin = animeSkin(skinColor);
    AnimeMaterial hair = animeHair(s.color.rgb);
    AnimeMaterial cloth = top == kTopPlate ? animeMetal(s.color2.rgb) : animeCloth(s.color2.rgb);
    AnimeMaterial trim = animeMetal(accent);
    AnimeMaterial bottomMat = bottom == kBottomGreaves ? animeMetal(bottomColor) : animeCloth(bottomColor);
    AnimeMaterial shoeMat = shoes == kShoesSabatons ? animeMetal(shoeColor) : animeMaterial(shoeColor);
    AnimeMaterial capeMat = animeCloth(capeColor);
    AnimeMaterial steel = animeMetal(animeHex(0xD2DCEEu));
    AnimeMaterial wood = animeMaterial(animeHex(0xA8703Eu));
    AnimeMaterial leather = animeMaterial(animeHex(0x7A4A2Eu));
    AnimeMaterial handMat = gloves == kGlovesNone ? skin : (gloves == kGlovesCloth ? animeCloth(accent) : steel);
    bool shortSleeve = top == kTopTee || top == kTopVest || top == kTopApron || top == kTopSailor;
    AnimeMaterial forearmMat = shortSleeve ? skin : cloth;
    gWeapon = weapon;
    gTop = top;
    gGloves = gloves;
    gAccent = accent;
    gTopColor = s.color2.rgb;
    gSleeve = top == kTopApron ? animeCloth(animeHex(0xF4EEE0u)) : cloth;
    gForearm = forearmMat;
    gHand = handMat;
    gTrim = trim;
    gSteel = steel;
    gWood = wood;
    gLeather = leather;
    gGem = animeGlow(mix(accent, vec3(1.0), 0.2));
    float sway = (pose == kPoseWalk ? wave : sin(time * 1.7 + seed * 4.0) * 0.5) * 2.0;
    vec2 head = r.head;
    float zipX = r.neck.x + turn * 6.0;

    // Head tilt: rotate the head's own anchor frame.
    // (hp: the pixel in the head's frame; the face, hair and hat use it.)
    vec2 hpx = rotate2d(p - head, -r.headTilt) + head;

    // ---- 1. cape
    if ((cape == kCapeCape || cape == kCapeMantle) && !prone) {
        vec2 a = r.neck + vec2(-5.0, 2.0);
        float len = cape == kCapeMantle ? 30.0 : 40.0;
        float capeD = sdLockBelly(p, a, a + vec2(-10.0 - sway, len * 0.4), a + vec2(-12.0 - sway * 2.0, len), 7.5, 9.5);
        capeD = max(capeD, -(p.x - r.neck.x - 2.0));
        animePart(C, capeD, capeMat);
        animeLine(C, max(sdBezier(p, a + vec2(-4.0, 10.0), a + vec2(-8.0, 20.0), a + vec2(-11.0 - sway * 2.0, len - 4.0)), capeD + 0.6),
                  capeMat.ink);
    }

    // ---- 2. back hair
    vec2 hb = hpx; // back hair in the head's frame
    {
        if (hairStyle == kHairPonytail) {
            vec2 tie = head + vec2(-17.0, -16.0);
            animePart(C, sdLockBelly(hb, tie, tie + vec2(-12.0 - sway, 6.0), tie + vec2(-14.0 - sway * 2.0, 30.0), 7.0, 1.2), hair);
            animePart(C, sdLock(hb, tie, tie + vec2(-5.0 - sway, 12.0), tie + vec2(-4.0 - sway * 2.0, 25.0), 4.5, 0.8), hair);
        } else if (hairStyle == kHairLong) {
            float curtain = sdLockBelly(hb, head + vec2(-6.0, -6.0), head + vec2(-15.0, 18.0), head + vec2(-13.0 - sway, 36.0), 19.0, 7.0);
            curtain = max(curtain, hb.x - head.x - 8.0);
            animePart(C, curtain, hair);
            animeLine(C, max(sdBezier(hb, head + vec2(-12.0, 12.0), head + vec2(-17.0, 24.0), head + vec2(-14.0 - sway, 34.0)), curtain + 0.8),
                      hair.ink);
        } else if (hairStyle == kHairBraid) {
            vec2 a = head + vec2(-14.0, 6.0);
            for (int i = 0; i < 4; ++i) {
                vec2 c = a + vec2(-1.5 - sway * 0.3 * float(i), float(i) * 6.0);
                animePart(C, sdEllipse(hb - c, vec2(4.6 - float(i) * 0.4, 3.8)), hair);
            }
            animePart(C, sdCircle(hb - a - vec2(-2.0 - sway, 25.0), 2.2), animeCloth(accent));
        } else if (hairStyle == kHairBob) {
            animePart(C, sdRoundBox(hb - head - vec2(-3.0, 2.5), vec2(23.0, 18.0), 13.0), hair);
        } else if (hairStyle == kHairBuns) {
            animePart(C, sdCircle(hb - head - vec2(-17.0, -19.0), 8.5), hair);
        } else if (hairStyle == kHairCurly) {
            for (int i = 0; i < 5; ++i) {
                float a = -2.6 + float(i) * 0.55;
                animePart(C, sdCircle(hb - head - vec2(-2.0, -5.0) - vec2(cos(a), sin(a)) * 23.0, 7.5), hair);
            }
        }
        float backHair = sdEllipse(hb - head - vec2(-2.0, -5.0), vec2(24.5, 23.5));
        if (hairStyle == kHairSprout || hairStyle == kHairSpiky) {
            // Soft nape locks curling out at the back of the neck.
            backHair = min(backHair, animeHairLock(hb, head + vec2(-14.0, -2.0), head + vec2(-22.0, 3.0), head + vec2(-25.0, 11.0), 6.0));
            backHair = min(backHair, animeHairLock(hb, head + vec2(-9.0, 6.0), head + vec2(-15.0, 11.0), head + vec2(-16.0, 16.0), 5.0));
        }
        animePart(C, backHair, hair);
    }

    // ---- 3. far arm
    AnimeMaterial sleeveFar = animeTinted(gSleeve, kFar);
    if (!climbing) {
        drawArm(r.shoulderF, r.elbowF, r.handF, top == kTopRobe, sleeveFar, animeTinted(forearmMat, kFar), animeTinted(handMat, kFar),
                gloves != kGlovesNone);
    }

    // ---- 4-6. legs and the bottom
    bool robe = top == kTopRobe || top == kTopCoat;
    AnimeMaterial shinMat = (bottom == kBottomPants || bottom == kBottomGreaves) && top != kTopRobe ? bottomMat : skin;
    AnimeMaterial thighMat = bottom == kBottomSkirt || top == kTopRobe ? skin : bottomMat;
    drawLeg(r.hipF, r.kneeF, r.footF, animeTinted(thighMat, kFar), animeTinted(shinMat, kFar), animeTinted(shoeMat, kFar), shoes, true);
    drawLeg(r.hipN, r.kneeN, r.footN, thighMat, shinMat, shoeMat, shoes, false);
    vec2 waist = vec2(r.neck.x + 0.5, r.hipN.y - 3.0);
    if (top != kTopRobe && !prone) {
        if (bottom == kBottomSkirt) {
            float flare = 1.5 + abs(wave) * 1.5;
            float skirt = sdTrapezoid(p - waist - vec2(0.5, 4.5), 13.5 + flare, 10.0, 5.5) - 1.5;
            animePart(C, skirt, bottomMat);
            for (int i = -1; i <= 2; ++i) {
                float x = waist.x + float(i) * 4.5 + 0.5;
                animeLine(C, max(sdSegment(p, vec2(x - float(i) * 0.5, waist.y + 0.5), vec2(x + float(i) * 0.8, waist.y + 10.5), 0.0), skirt + 0.6),
                          bottomMat.ink);
            }
        } else {
            float len = bottom == kBottomShorts ? 4.5 : 3.5;
            animePart(C, sdTrapezoid(p - waist - vec2(0.5, len), 12.0, 10.5, len) - 1.5, bottomMat);
        }
    }

    // ---- 7. the top
    vec2 chest = vec2(r.neck.x + 0.8, (r.neck.y + r.hipN.y) * 0.5 - 1.0);
    float torsoHalf = (r.hipN.y - r.neck.y) * 0.5;
    if (prone) {
        // Lying down: the body is a rounded log from shoulders to hips.
        float body = sdSegment(p, r.shoulderN + vec2(-1.0, 0.0), r.hipN + vec2(2.0, 0.0), 6.5);
        animePart(C, body, cloth);
        animePart(C, max(body, abs(p.x - (r.hipN.x + 6.0)) - 2.0), leather);
    } else if (top == kTopRobe) {
        float robeD = sdTrapezoid(p - vec2(chest.x + 0.5, (r.neck.y + 51.0) * 0.5), 15.5 + abs(wave) * 1.5, 11.0,
                                  (51.0 - r.neck.y) * 0.5) - 3.0;
        animePart(C, robeD, cloth);
        float hemY = 50.0;
        animePartNoLine(C, max(robeD, -(p.y - hemY)), trim);
        animeLine(C, max(abs(p.y - hemY), robeD), cloth.ink);
        float front = abs(p.x - zipX - (p.y - r.neck.y) * 0.12) - 1.4;
        animePartNoLine(C, max(robeD, front), trim);
        animeLine(C, max(abs(front), robeD), cloth.ink);
        animeLine(C, max(sdBezier(p, vec2(chest.x - 6.0, 30.0), vec2(chest.x - 8.0, 40.0), vec2(chest.x - 10.0, 50.0)), robeD + 0.6),
                  cloth.ink);
        animePart(C, sdRoundBox(p - vec2(zipX + 1.0, r.hipN.y - 4.0), vec2(12.5, 1.6), 1.6), animeMaterial(accent));
        // A round collar.
        animePart(C, max(sdEllipse(p - r.neck - vec2(turn * 2.0, 1.5), vec2(9.0, 4.0)), -robeD - 20.0), trim);
    } else {
        float bulge = top == kTopPlate ? 1.0 : 0.0;
        float torso = sdTrapezoid(p - chest, 8.0 + abs(wave) * 0.4 + bulge, 9.5 + bulge, torsoHalf - 1.0) - 3.2;
        if (top == kTopCoat) {
            // A long coat: flares past the hips.
            torso = min(torso, sdTrapezoid(p - vec2(chest.x, r.hipN.y + 4.0), 14.0 + abs(wave) * 1.0, 12.0, 6.0) - 2.0);
        }
        animePart(C, torso, cloth);
        float hemY = r.hipN.y + 1.0;
        if (top == kTopTee) {
            animeLine(C, max(sdBezier(p, vec2(zipX - 4.5, r.neck.y + 0.5), vec2(zipX, r.neck.y + 3.5), vec2(zipX + 4.0, r.neck.y + 0.5)), -torso),
                      cloth.ink);
            // A star on the chest (the island's mark).
            animePart(C, sdStar(p - vec2(zipX + 1.0, chest.y + 1.5), 5, 3.4, 1.6) - 0.3, animeMaterial(accent));
        } else if (top == kTopTunic) {
            animePartNoLine(C, max(torso, -(p.y - hemY + 2.5)), trim);
            animeLine(C, max(abs(p.y - hemY + 2.5), torso), cloth.ink);
            animeLine(C, max(sdBezier(p, vec2(zipX - 5.0, r.neck.y + 0.5), vec2(zipX, r.neck.y + 6.5), vec2(zipX + 4.0, r.neck.y + 0.5)), -torso),
                      cloth.ink);
        } else if (top == kTopVest || top == kTopCoat) {
            float shirt = max(torso, abs(p.x - zipX) - 3.2);
            animePartNoLine(C, shirt, animeCloth(animeHex(0xF6F0E2u)));
            animeLine(C, max(abs(abs(p.x - zipX) - 3.2), torso), cloth.ink);
            for (int i = 0; i < 2; ++i) {
                animePart(C, sdCircle(p - vec2(zipX + 4.5, r.neck.y + 6.0 + float(i) * 6.0), 1.0), trim);
            }
            if (top == kTopCoat) {
                animePartNoLine(C, max(torso, -(p.y - r.hipN.y - 7.5)), trim);
                animeLine(C, max(abs(p.y - r.hipN.y - 7.5), torso), cloth.ink);
            }
        } else if (top == kTopPlate) {
            animeLine(C, max(abs(p.x - zipX), torso + 2.0), cloth.ink);
            animeLine(C, max(abs(p.y - r.neck.y - 10.0), torso), cloth.ink);
            animeGloss(C, p, torso, vec2(zipX - 5.0, r.neck.y + 5.5), vec2(4.0, 3.0), 0.55);
            animePart(C, sdCircle(p - vec2(zipX, r.neck.y + 5.5), 1.6), gGem);
        } else if (top == kTopApron) {
            // A shirt with a front apron and a pocket (shopkeepers).
            AnimeMaterial apron = animeCloth(mix(accent, vec3(1.0), 0.55));
            float a = max(torso + 0.8, sdRoundBox(p - vec2(zipX, chest.y + 4.0), vec2(7.5, torsoHalf + 1.0), 3.0));
            animePart(C, a, apron);
            animePart(C, sdRoundBox(p - vec2(zipX, chest.y + 6.0), vec2(4.0, 2.4), 1.2), apron);
        } else {
            // Sailor blouse: a square collar with a stripe and a ribbon.
            AnimeMaterial navy = animeCloth(animeHex(0x3D4C92u));
            float collarShape = sdTrapezoid(p - vec2(zipX - 1.5, r.neck.y + 3.5), 11.5, 9.0, 3.5);
            float collar = max(collarShape, torso);
            animePart(C, collar, navy);
            animeLine(C, max(abs(collarShape + 1.3), collar), vec3(0.9));
            vec2 knot = vec2(zipX, r.neck.y + 7.0);
            AnimeMaterial ribbon = animeCloth(accent);
            animePart(C, sdLock(p, knot, knot + vec2(-2.6, 2.6), knot + vec2(-2.2, 7.0), 1.8, 0.9), ribbon);
            animePart(C, sdLock(p, knot, knot + vec2(2.6, 2.6), knot + vec2(3.0, 7.0), 1.8, 0.9), ribbon);
            animePart(C, sdCircle(p - knot, 1.8), ribbon);
        }
        if (top == kTopTunic || top == kTopPlate || top == kTopVest || top == kTopTee) {
            float beltY = r.hipN.y - 3.5;
            float belt = max(torso, abs(p.y - beltY) - 1.6);
            animePart(C, belt, leather);
            animePart(C, sdRoundBox(p - vec2(zipX, beltY), vec2(2.0, 2.0), 0.6), trim);
        }
    }

    // ---- 8. neck and head (the whole head turns with its tilt)
    vec2 hp = hpx;
    float neckD = sdSegment(p, r.neck + vec2(0.0, -4.0), r.neck + vec2(0.0, 1.0), 3.4);
    animePart(C, neckD, skin);
    animeCastShadow(C, neckD, faceShape(hp - vec2(0.8, 3.0), head, turn), skin);
    float face = faceShape(hp, head, turn);
    animePart(C, face, skin);
    if (turn > 0.15 && hairStyle != kHairLong && hairStyle != kHairBob) {
        float ear = sdEllipse(hp - head - vec2(-12.5, 4.5), vec2(3.3, 4.3));
        animePart(C, max(ear, -face - 0.2), skin);
    }
    vec2 hc = head + vec2(turn * 2.0, 0.0);

    // ---- 9. face
    if (!climbing) {
        vec2 drop = vec2(1.0, 2.6);
        animeCastShadow(C, face, min(fringeShape(hp - drop, hc, hairStyle), capShape(hp - drop, head, hairStyle)), skin);
        AnimeFace f = animeFaceMix(animeExpression(exprA), animeExpression(exprB), blend);
        if (pose == kPoseHurt) {
            f = animeExpression(kAnimeHurt);
        } else if (pose == kPoseSwing || pose == kPoseCast || pose == kPoseThrow) {
            // Attacks always read determined: open, narrowed eyes and set brows.
            f = animeExpression(kAnimeDetermined);
        } else if (pose == kPoseCheer) {
            f = animeExpression(kAnimeHappy);
        }
        if (f.eyeShape == kAnimeEyeOpen) {
            f.open *= animeBlink(time, seed);
            if (f.open < 0.12) {
                f.eyeShape = kAnimeEyeClosed;
            }
        }
        // Chibi: big eyes low on the face, a tiny mouth.
        AnimeCanvas fc = C;
        animeFaceDraw(fc, hp, head + vec2(0.0, 0.5), 52.0, turn, 1.0, f, iris, skinColor);
        C = fc;
    }

    // ---- 10. front hair with the sheen band
    float cap = capShape(hp, head, hairStyle);
    float fringe = climbing ? sdEllipse(hp - head - vec2(0.0, -2.0), vec2(22.0, 21.0)) : fringeShape(hp, hc, hairStyle);
    float shine = animeHairShine(C, hp, head + vec2(-1.0, -6.0), 17.5, 1.8) * 0.55;
    vec3 sheen = mix(s.color.rgb, vec3(1.0), 0.34);
    {
        animePartShine(C, cap, hair, shine, sheen);
        animePartShine(C, fringe, hair, climbing ? 0.0 : shine, sheen);
        animeLine(C, max(sdBezier(hp, hc + vec2(-1.0, -28.0), hc + vec2(-5.0, -21.0), hc + vec2(-8.0, -14.0)), cap + 0.8), hair.ink);
        animeLine(C, max(sdBezier(hp, hc + vec2(7.0, -27.0), hc + vec2(9.0, -21.0), hc + vec2(10.0, -14.0)), cap + 0.8), hair.ink);
    }
    if (hairStyle == kHairSprout) {
        // The sprout: a little curl standing up on the crown.
        vec2 base = head + vec2(2.0, -27.5);
        animePart(C, sdLock(hp, base, base + vec2(2.0, -6.0 + sway * 0.3), base + vec2(-3.5, -9.0), 2.6, 0.6), hair);
    } else if (hairStyle == kHairPonytail) {
        animePart(C, sdCircle(hp - head - vec2(-17.0, -16.0), 3.0), animeMaterial(accent));
    } else if (hairStyle == kHairBuns) {
        vec2 bun = head + vec2(13.0, -22.0);
        animePartShine(C, sdCircle(hp - bun, 7.5), hair, animeHairShine(C, hp, bun, 4.5, 1.6), sheen);
        animePart(C, sdCircle(hp - head - vec2(-17.0, -19.0) - vec2(4.0, 4.0), 2.2), animeMaterial(accent));
    } else if (hairStyle == kHairSpiky) {
        vec2 base = head + vec2(-6.0, -26.0);
        animePart(C, animeHairLock(hp, base, base + vec2(-6.0, -5.0), base + vec2(-13.0, -4.0), 5.0), hair);
    }

    // ---- 11. hat
    AnimeMaterial hatMat = animeCloth(hatColor);
    vec2 hh = rotate2d(hp - head, 0.025 * turn) + head;
    if (hat == kHatWizard) {
        vec2 brim = head + vec2(turn * 5.0, -24.0);
        vec2 radii = vec2(hh.x < brim.x ? 23.0 : 28.0, 5.0);
        animePart(C, sdEllipse(hh - brim, radii), hatMat);
        vec2 base = brim + vec2(1.0, -1.0);
        float crown = sdTrapezoid(hh - base - vec2(0.0, -5.5), 13.0, 9.5, 5.5) - 1.0;
        vec2 tip = base + vec2(-14.0 - sway * 0.5, -23.0);
        crown = animeSmoothUnion(crown, sdLock(hh, base + vec2(0.0, -9.0), base + vec2(2.0, -22.0), tip, 9.5, 1.5), 3.0);
        animePart(C, crown, hatMat);
        float band = max(crown + 0.5, abs(hh.y - base.y + 3.8) - 2.4);
        animePartNoLine(C, band, trim);
        animeLine(C, max(abs(abs(hh.y - base.y + 3.8) - 2.4), crown), hatMat.ink);
        animePart(C, sdStar(hh - tip, 5, 3.6, 1.7) - 0.3, trim);
    } else if (hat == kHatBeret) {
        // A soft beret leaning back with a stalk.
        vec2 bc = head + vec2(-3.0 + turn * 2.0, -24.0);
        float beret = sdEllipse(rotate2d(hh - bc, -0.25), vec2(21.0, 8.5));
        animePart(C, beret, hatMat);
        animePart(C, max(sdEllipse(rotate2d(hh - bc - vec2(2.0, 4.5), -0.25), vec2(19.0, 3.0)), beret - 0.5), animeTinted(hatMat, vec3(0.85)));
        animePart(C, sdSegment(hh, bc + vec2(1.0, -7.5), bc + vec2(2.0, -11.0), 1.2), hatMat);
    } else if (hat == kHatBandana) {
        float band = max(cap - 0.5, abs(hp.y - (head.y - 14.0 - (hp.x - head.x) * 0.1)) - 2.2);
        animePart(C, band, hatMat);
        vec2 knot = head + vec2(-21.0, -13.0);
        animePart(C, sdLock(hp, knot, knot + vec2(-6.0, 2.0 - sway), knot + vec2(-12.0, 6.0 - sway), 2.2, 0.8), hatMat);
        animePart(C, sdLock(hp, knot, knot + vec2(-5.0, 5.0 - sway), knot + vec2(-8.0, 11.0 - sway), 2.0, 0.7), hatMat);
    } else if (hat == kHatHelmet) {
        vec2 hb2 = head + vec2(turn * 2.5 - 1.0, -8.0);
        AnimeMaterial metal = animeMetal(hatColor);
        float helm = max(sdCircle(hh - hb2, 25.0), hh.y - head.y + 10.5);
        animePartShine(C, helm, metal, animeCover(C, abs(sdCircle(hh - hb2 - vec2(-3.0, -6.0), 15.0)) - 1.0) * 0.7, vec3(1.0));
        float rim = max(sdCircle(hh - hb2, 26.0), abs(hh.y - head.y + 12.0) - 2.2);
        animePart(C, rim, metal);
        animeLine(C, max(abs(hh.x - hb2.x - turn * 9.0), helm + 1.5), metal.ink);
        float plume = sdLockBelly(hh, hb2 + vec2(-2.0, -23.0), hb2 + vec2(-15.0, -33.0 - sway), hb2 + vec2(-28.0, -21.0), 4.5, 1.0);
        animePart(C, plume, animeCloth(accent));
    } else if (hat == kHatFeather) {
        // A traveler's cap with a long feather.
        vec2 hb2 = head + vec2(turn * 2.0, -10.0);
        float capD = max(sdEllipse(hh - hb2 - vec2(0.0, -6.0), vec2(24.5, 18.0)), hh.y - head.y + 14.0);
        animePart(C, capD, hatMat);
        float brim = sdRoundBox(hh - head - vec2(4.0 + turn * 3.0, -14.0), vec2(22.0, 2.0), 2.0);
        animePart(C, brim, animeTinted(hatMat, vec3(0.88)));
        animePart(C, sdLock(hh, head + vec2(-10.0, -22.0), head + vec2(-22.0, -34.0 - sway), head + vec2(-34.0, -30.0 - sway), 3.6, 0.5),
                  animeCloth(accent));
    } else if (hat == kHatFlower) {
        for (int i = 0; i < 5; ++i) {
            float x = -14.0 + float(i) * 7.0;
            vec2 at = head + vec2(x + turn * 2.0, -21.0 + abs(x) * abs(x) * 0.022);
            vec3 petal = i % 2 == 0 ? hatColor : animeHex(0xFFF2F6u);
            animePart(C, sdStar(hp - at, 5, 3.8, 2.3) - 0.6, animeCloth(petal));
            animeFlat(C, sdCircle(hp - at, 1.1), animeHex(0xFAD65Eu));
        }
    } else if (hat == kHatHood) {
        float hood = sdEllipse(hh - head - vec2(-1.5, -5.0), vec2(27.0, 26.5));
        hood = max(hood, -(sdEllipse(hh - head - vec2(3.0, 2.0), vec2(20.0, 20.0))));
        animePart(C, hood, hatMat);
        animeLine(C, max(sdBezier(hh, head + vec2(-17.0, -10.0), head + vec2(-22.0, 6.0), head + vec2(-14.0, 18.0)), hood + 0.6), hatMat.ink);
    }
    if (cape == kCapeScarf && !prone) {
        float wrap = sdRoundBox(p - r.neck - vec2(1.0, 0.5), vec2(8.0, 3.0), 3.0);
        float tail = sdLock(p, r.neck + vec2(-6.0, 1.0), r.neck + vec2(-13.0, 2.0 + sway), r.neck + vec2(-20.0 - sway, 7.0 + sway), 3.0, 1.2);
        animePart(C, tail, capeMat);
        animePart(C, wrap, capeMat);
    } else if (cape == kCapeMantle && !prone) {
        float mantle = max(sdEllipse(p - r.neck - vec2(0.5, 4.0), vec2(14.0, 7.5)), -(p.y - r.neck.y + 1.0));
        animePart(C, mantle, capeMat);
        animePart(C, sdCircle(p - r.neck - vec2(turn * 5.0, 2.5), 2.0), trim);
    }

    // ---- 12. weapon and near arm (climbing: both arms over the head)
    if (climbing) {
        drawArm(r.shoulderF, r.elbowF, r.handF, top == kTopRobe, animeTinted(gSleeve, kFar), animeTinted(forearmMat, kFar),
                animeTinted(handMat, kFar), gloves != kGlovesNone);
        drawArm(r.shoulderN, r.elbowN, r.handN, top == kTopRobe, gSleeve, forearmMat, handMat, gloves != kGlovesNone);
    } else {
        drawWeaponAndNearArm(r, pose, ph, time);
    }

    outColor = animeFinish(C);

    if (glow) {
        // A buff's soft halo hugging the silhouette.
        float halo = glowFalloff(C.body - C.outline, 7.0) * (0.55 + 0.25 * sin(time * 5.0));
        outColor = over(outColor, vec4(gAccent * 1.6, halo * (1.0 - outColor.a)));
    }
    if (ghost) {
        // An afterimage: the silhouette in the tint, no detail.
        outColor = vec4(s.color2.rgb, outColor.a * 0.8);
    }

    if (debugRig) {
        vec4 dbg = vec4(0.0);
        float lw = 1.2 * C.px;
        float bones = min(min(sdSegment(p, r.shoulderN, r.elbowN, 0.0), sdSegment(p, r.elbowN, r.handN, 0.0)),
                          min(sdSegment(p, r.shoulderF, r.elbowF, 0.0), sdSegment(p, r.elbowF, r.handF, 0.0)));
        bones = min(bones, min(min(sdSegment(p, r.hipN, r.kneeN, 0.0), sdSegment(p, r.kneeN, r.footN - vec2(1.0, 3.5), 0.0)),
                               min(sdSegment(p, r.hipF, r.kneeF, 0.0), sdSegment(p, r.kneeF, r.footF - vec2(1.0, 3.5), 0.0))));
        bones = min(bones, min(sdSegment(p, r.neck, (r.hipN + r.hipF) * 0.5, 0.0), sdSegment(p, r.neck, r.head, 0.0)));
        dbg = over(vec4(vec3(1.0, 0.25, 0.6), animeCover(C, bones - lw)), dbg);
        outColor = over(dbg, outColor);
    }
    outColor.a *= s.color.a;
}
