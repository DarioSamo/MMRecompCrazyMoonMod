#include "modding.h"
#include "global.h"
#include "math.h"
#include "overlays\actors\ovl_En_Fall\z_en_fall.h"

RECOMP_IMPORT("*", int recomp_printf(const char* fmt, ...));

Actor* EnFall_MoonsTear_GetTerminaFieldMoon(PlayState* play);

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonPerform_Args;

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonAdjust_Args;

struct {
    EnFall* this;
    PlayState* play;
} EnFall_MoonSetup_Args;

#define G_MOON_SFX_MAX 64

float g_moonTime = 0.0f;
float g_moonSfxVolumes[G_MOON_SFX_MAX];
float g_moonSfxFrequencies[G_MOON_SFX_MAX];
u32 g_moonSfxIndex = 0;
u32 g_moonSfxTimer = 0;
Vec3f g_moonScale = { 1.0f, 1.0f, 1.0f };

bool EnFall_CrashingMoon_IsMoonType(EnFall* this) {
    switch (EN_FALL_TYPE(&this->actor)) {
        case EN_FALL_TYPE_CRASH_FIRE_BALL:
        case EN_FALL_TYPE_CRASH_RISING_DEBRIS:
        case EN_FALL_TYPE_MOONS_TEAR:
        case EN_FALL_TYPE_CRASH_FIRE_RING:
            return false;
        default:
            return true;
    }
}

void EnFall_CrashingMoon_PerformActionsCommonHook(EnFall* this, PlayState* play) {
    if (!EnFall_CrashingMoon_IsMoonType(this)) {
        return;
    }

    Actor* terminaFieldMoon = EnFall_MoonsTear_GetTerminaFieldMoon(play);
    if ((terminaFieldMoon != NULL) && (&this->actor != terminaFieldMoon)) {
        // Ignore the ghost crashing moon on Termina Field.
        return;
    }

    float dayProgress = MAX(((f32)(CURRENT_TIME) - (f32)(CLOCK_TIME(6, 0))) * (1.0f / 0x10000), 0.0f);
    float intensity = 0.001f + ((f32)(CURRENT_DAY) + dayProgress) / 3.0f;
    const float TwoPi = M_PI * 2.0f;
    g_moonTime += intensity;
    while (g_moonTime >= TwoPi) {
        g_moonTime -= TwoPi;
    }

    Vec3f eyeToWorld, eyeToAt;
    Math_Vec3f_Diff(&this->actor.world.pos, &play->view.eye, &eyeToWorld);
    Math_Vec3f_Diff(&play->view.at, &play->view.eye, &eyeToAt);

    float vectorCos = MAX(Math3D_Cos(&eyeToWorld, &eyeToAt), 0.0f);
    float moonScale = Math_PowF(vectorCos, 4.0f);
    float sfxScale = Math_PowF(vectorCos, 6.0f);

    // Restore the stored scale first before scaling the moon.
    this->actor.scale = g_moonScale;
    Math_Vec3f_Scale(&this->actor.scale, 1.0f + (cosf(g_moonTime) * 0.5f) * moonScale);

    float volume = MIN(2.0f * vectorCos * intensity, 1.0f);
    if (volume > 0.25f) {
        if (g_moonSfxTimer == 0) {
            u32 sfxRandom = Rand_Next() % 100;
            u32 sfxId = NA_SE_VO_LI_DAMAGE_S;
            if (sfxRandom > 95) {
                sfxId = NA_SE_VO_LI_FALL_L;
            }
            else if (sfxRandom > 90) {
                sfxId = NA_SE_VO_LI_GROAN;
            }

            Player* player = GET_PLAYER(play);
            g_moonSfxVolumes[g_moonSfxIndex] = volume;
            g_moonSfxFrequencies[g_moonSfxIndex] = 0.6f + Rand_ZeroOne() * 0.2f;
            AudioSfx_PlaySfx(sfxId, &player->actor.projectedPos, 4, &g_moonSfxFrequencies[g_moonSfxIndex], &g_moonSfxVolumes[g_moonSfxIndex], &gSfxDefaultReverb);
            g_moonSfxTimer = (u32)(Math_FRoundF(1.0f + ((0.5f + Rand_ZeroOne() * 0.5f) / MAX(sfxScale * intensity, 0.05f))));
            g_moonSfxIndex = (g_moonSfxIndex + 1) % G_MOON_SFX_MAX;
        }
        else {
            g_moonSfxTimer--;
        }
    }
}

void EnFall_CrashingMoon_StoreScaleHook(EnFall* this, PlayState* play) {
    if (Object_IsLoaded(&play->objectCtx, this->objectSlot) && EnFall_CrashingMoon_IsMoonType(this)) {
        g_moonScale = this->actor.scale;
    }
}

RECOMP_HOOK("EnFall_CrashingMoon_PerformCutsceneActions") void EnFall_CrashingMoon_PerformCutsceneActionsHook(EnFall* this, PlayState* play) {
    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_CrashingMoon_PerformCutsceneActions") void EnFall_CrashingMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(EnFall_MoonPerform_Args.this, EnFall_MoonPerform_Args.play);
}

RECOMP_HOOK("EnFall_StoppedOpenMouthMoon_PerformCutsceneActions") void EnFall_StoppedOpenMouthMoon_PerformCutsceneActionsHook(EnFall* this, PlayState* play) {
    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_StoppedOpenMouthMoon_PerformCutsceneActions") void EnFall_StoppedOpenMouthMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(EnFall_MoonPerform_Args.this, EnFall_MoonPerform_Args.play);
}

RECOMP_HOOK("EnFall_StoppedClosedMouthMoon_PerformCutsceneActions") void EnFall_StoppedClosedMouthMoon_PerformCutsceneActionsHook(EnFall* this, PlayState* play) {
    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_StoppedClosedMouthMoon_PerformCutsceneActions") void EnFall_StoppedClosedMouthMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(EnFall_MoonPerform_Args.this, EnFall_MoonPerform_Args.play);
}

RECOMP_HOOK("EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActions") void EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActionsHook(EnFall* this, PlayState* play) {
    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActions") void EnFall_ClockTowerOrTitleScreenMoon_PerformCutsceneActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(EnFall_MoonPerform_Args.this, EnFall_MoonPerform_Args.play);
}

RECOMP_HOOK("EnFall_Moon_PerformDefaultActions") void EnFall_Moon_PerformDefaultActionsHook(EnFall* this, PlayState* play) {
    EnFall_MoonPerform_Args.this = this;
    EnFall_MoonPerform_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Moon_PerformDefaultActions") void EnFall_Moon_PerformDefaultActionsHookReturn() {
    EnFall_CrashingMoon_PerformActionsCommonHook(EnFall_MoonPerform_Args.this, EnFall_MoonPerform_Args.play);
}

RECOMP_HOOK("EnFall_Moon_AdjustScaleAndPosition") void EnFall_Moon_AdjustScaleAndPositionHook(EnFall* this, PlayState* play) {
    EnFall_MoonAdjust_Args.this = this;
    EnFall_MoonAdjust_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Moon_AdjustScaleAndPosition") void EnFall_Moon_AdjustScaleAndPositionHookReturn() {
    EnFall_CrashingMoon_StoreScaleHook(EnFall_MoonAdjust_Args.this, EnFall_MoonAdjust_Args.play);
}

RECOMP_HOOK("EnFall_Setup") void EnFall_SetupHook(EnFall* this, PlayState* play) {
    EnFall_MoonSetup_Args.this = this;
    EnFall_MoonSetup_Args.play = play;
}

RECOMP_HOOK_RETURN("EnFall_Setup") void EnFall_SetupHookReturn() {
    EnFall_CrashingMoon_StoreScaleHook(EnFall_MoonSetup_Args.this, EnFall_MoonSetup_Args.play);
}
