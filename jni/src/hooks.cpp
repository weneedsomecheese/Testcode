#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

// ============================================================================
// GAME-SPECIFIC HOOKS
//
// Target classes (from dump.cs):
//   CCharUser       — local player character (inherits CCharPlayer -> CCharBase)
//   iDataCenter     — persistent save data (gold stored as SafeInteger at 0x10)
//   CNameCardInfo   — player info (gold property at get/set_m_nGold)
//   MyUtils          — static gold/exp formula functions
//
// SafeInteger is the game's anti-cheat integer wrapper.
//   SafeInteger.Get()        — RVA: 0xC16050
//   SafeInteger.Set(int)     — RVA: 0xC15FF8
// ============================================================================

enum Toggle {
    TOGGLE_GOLD_MULTIPLY = 0,
    TOGGLE_GOD_MODE      = 1,
    TOGGLE_ONE_HIT       = 2,
};

// --- Gold multiplier ---
// CCharUser.AddGold(SafeInteger nGold) — RVA: 0xA4FFB0
// When gold is added to the player, multiply the amount.
typedef void (*CCharUser_AddGold_t)(void *self, void *nGold, void *method);
static CCharUser_AddGold_t orig_CCharUser_AddGold = nullptr;

// SafeInteger methods resolved at runtime
typedef int (*SafeInteger_Get_t)(void *self, void *method);
typedef void (*SafeInteger_Set_t)(void *self, int value, void *method);
static SafeInteger_Get_t SafeInteger_Get = nullptr;
static SafeInteger_Set_t SafeInteger_Set = nullptr;

void hook_CCharUser_AddGold(void *self, void *nGold, void *method) {
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY) && nGold && SafeInteger_Get && SafeInteger_Set) {
        int original_gold = SafeInteger_Get(nGold, nullptr);
        int multiplier = menu::get_slider(0);
        if (multiplier < 2) multiplier = 10;
        int new_gold = original_gold * multiplier;
        SafeInteger_Set(nGold, new_gold, nullptr);
        LOGI("Gold multiplied: %d -> %d (x%d)", original_gold, new_gold, multiplier);
    }
    orig_CCharUser_AddGold(self, nGold, method);
}

// --- iDataCenter.AddGold(int nGold) — RVA: 0xA95E88 ---
// This is the persistent save data gold add. Multiplying here ensures
// the gold actually gets saved.
typedef void (*iDataCenter_AddGold_t)(void *self, int nGold, void *method);
static iDataCenter_AddGold_t orig_iDataCenter_AddGold = nullptr;

void hook_iDataCenter_AddGold(void *self, int nGold, void *method) {
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY)) {
        int multiplier = menu::get_slider(0);
        if (multiplier < 2) multiplier = 10;
        LOGI("iDataCenter gold multiplied: %d -> %d (x%d)", nGold, nGold * multiplier, multiplier);
        nGold *= multiplier;
    }
    orig_iDataCenter_AddGold(self, nGold, method);
}

// --- Gold formula hooks ---
// MyUtils.formula_monstergold(int nGold, int nLevel) — RVA: 0xBCB828
// MyUtils.formula_stagegold(int nGold, int nLevel)   — RVA: 0xBCBB18
// These calculate how much gold monsters/stages give. Multiply the result.
typedef int (*formula_gold_t)(int nGold, int nLevel, void *method);
static formula_gold_t orig_formula_monstergold = nullptr;
static formula_gold_t orig_formula_stagegold = nullptr;

int hook_formula_monstergold(int nGold, int nLevel, void *method) {
    int result = orig_formula_monstergold(nGold, nLevel, method);
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY)) {
        int multiplier = menu::get_slider(0);
        if (multiplier < 2) multiplier = 10;
        result *= multiplier;
    }
    return result;
}

int hook_formula_stagegold(int nGold, int nLevel, void *method) {
    int result = orig_formula_stagegold(nGold, nLevel, method);
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY)) {
        int multiplier = menu::get_slider(0);
        if (multiplier < 2) multiplier = 10;
        result *= multiplier;
    }
    return result;
}

// --- God mode ---
// CCharUser.OnHit(float fDmg, CWeaponInfoLevel, string) — RVA: 0xA4F3D0
typedef bool (*OnHit_t)(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method);
static OnHit_t orig_OnHit = nullptr;

bool hook_OnHit(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method) {
    if (menu::get_toggle(TOGGLE_GOD_MODE)) {
        return false;
    }
    return orig_OnHit(self, fDmg, pWeaponLvlInfo, sBodyPart, method);
}

// --- One-hit kill ---
// CCharMob.OnHit(float fDmg, CWeaponInfoLevel, string) — need to find RVA
// We hook the mob's OnHit and set damage to a massive value
static OnHit_t orig_MobOnHit = nullptr;

bool hook_MobOnHit(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method) {
    if (menu::get_toggle(TOGGLE_ONE_HIT)) {
        fDmg = 999999.0f;
    }
    return orig_MobOnHit(self, fDmg, pWeaponLvlInfo, sBodyPart, method);
}

// ============================================================================
// HOOK INSTALLATION
// ============================================================================

static void resolve_safe_integer() {
    uintptr_t base = il2cpp::get_base_address();
    SafeInteger_Get = reinterpret_cast<SafeInteger_Get_t>(base + 0xC16050);
    SafeInteger_Set = reinterpret_cast<SafeInteger_Set_t>(base + 0xC15FF8);
    LOGI("SafeInteger.Get: %p, SafeInteger.Set: %p",
         (void *)SafeInteger_Get, (void *)SafeInteger_Set);
}

void install_hooks() {
    uintptr_t base = il2cpp::get_base_address();
    resolve_safe_integer();

    // Gold hooks
    hook::hook_addr(base + 0xA4FFB0,
                    (void *)hook_CCharUser_AddGold,
                    (void **)&orig_CCharUser_AddGold);
    LOGI("Hooked CCharUser.AddGold @ 0xA4FFB0");

    hook::hook_addr(base + 0xA95E88,
                    (void *)hook_iDataCenter_AddGold,
                    (void **)&orig_iDataCenter_AddGold);
    LOGI("Hooked iDataCenter.AddGold @ 0xA95E88");

    hook::hook_addr(base + 0xBCB828,
                    (void *)hook_formula_monstergold,
                    (void **)&orig_formula_monstergold);
    LOGI("Hooked MyUtils.formula_monstergold @ 0xBCB828");

    hook::hook_addr(base + 0xBCBB18,
                    (void *)hook_formula_stagegold,
                    (void **)&orig_formula_stagegold);
    LOGI("Hooked MyUtils.formula_stagegold @ 0xBCBB18");

    // God mode
    hook::hook_addr(base + 0xA4F3D0,
                    (void *)hook_OnHit,
                    (void **)&orig_OnHit);
    LOGI("Hooked CCharUser.OnHit @ 0xA4F3D0");

    LOGI("=== All hooks installed ===");
    LOGI("Toggle 0: Gold Multiplier (slider 0 = multiplier, default 10x)");
    LOGI("Toggle 1: God Mode");
    LOGI("Toggle 2: One-Hit Kill");
}
