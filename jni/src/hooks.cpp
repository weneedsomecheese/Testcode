#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

// ============================================================================
// GAME-SPECIFIC HOOKS — All RVAs from dump.cs
//
// Class hierarchy:
//   CCharBase (virtual base)
//     CCharMob         — enemy/monster characters
//     CCharPlayer      — base player (networked)
//       CCharPlayerNet — other networked players
//       CCharUser      — local player
//
//   CWeaponBase        — weapon system (bullets, firing)
//   iDataCenter        — persistent save data (gold, crystal, inventory)
//   MyUtils            — static formula functions (gold/exp/damage scaling)
//   SafeInteger        — anti-cheat encrypted integer wrapper
// ============================================================================

// --- Toggle indices for the mod menu ---
enum Toggle {
    TOGGLE_GOLD_MULTIPLY    = 0,
    TOGGLE_CRYSTAL_MULTIPLY = 1,
    TOGGLE_GOD_MODE         = 2,
    TOGGLE_ONE_HIT          = 3,
    TOGGLE_UNLIMITED_AMMO   = 4,
    TOGGLE_EXP_MULTIPLY     = 5,
    TOGGLE_DAMAGE_MULTIPLY  = 6,
};

// Slider indices:
//   0 = gold/crystal/exp multiplier (default 10x)
//   1 = damage multiplier (default 10x)

// ============================================================================
// SafeInteger helpers
// ============================================================================
typedef int  (*SafeInteger_Get_t)(void *self, void *method);
typedef void (*SafeInteger_Set_t)(void *self, int value, void *method);
static SafeInteger_Get_t  SafeInteger_Get = nullptr;
static SafeInteger_Set_t  SafeInteger_Set = nullptr;

static int get_multiplier(int slider_index, int default_val = 10) {
    int m = menu::get_slider(slider_index);
    return (m < 2) ? default_val : m;
}

// ============================================================================
// GOLD HOOKS
// ============================================================================

// CCharUser.AddGold(SafeInteger nGold) — RVA: 0xA4FFB0
typedef void (*CCharUser_AddGold_t)(void *self, void *nGold, void *method);
static CCharUser_AddGold_t orig_CCharUser_AddGold = nullptr;

void hook_CCharUser_AddGold(void *self, void *nGold, void *method) {
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY) && nGold && SafeInteger_Get && SafeInteger_Set) {
        int orig = SafeInteger_Get(nGold, nullptr);
        int mult = get_multiplier(0);
        SafeInteger_Set(nGold, orig * mult, nullptr);
        LOGI("CCharUser gold: %d -> %d (x%d)", orig, orig * mult, mult);
    }
    orig_CCharUser_AddGold(self, nGold, method);
}

// iDataCenter.AddGold(int nGold) — RVA: 0xA95E88
typedef void (*iDataCenter_AddGold_t)(void *self, int nGold, void *method);
static iDataCenter_AddGold_t orig_iDataCenter_AddGold = nullptr;

void hook_iDataCenter_AddGold(void *self, int nGold, void *method) {
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY)) {
        int mult = get_multiplier(0);
        LOGI("iDataCenter gold: %d -> %d (x%d)", nGold, nGold * mult, mult);
        nGold *= mult;
    }
    orig_iDataCenter_AddGold(self, nGold, method);
}

// MyUtils.formula_monstergold(int nGold, int nLevel) — RVA: 0xBCB828
// MyUtils.formula_stagegold(int nGold, int nLevel) — RVA: 0xBCBB18
typedef int (*formula_t)(int nValue, int nLevel, void *method);
static formula_t orig_formula_monstergold = nullptr;
static formula_t orig_formula_stagegold = nullptr;

int hook_formula_monstergold(int nGold, int nLevel, void *method) {
    int result = orig_formula_monstergold(nGold, nLevel, method);
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY))
        result *= get_multiplier(0);
    return result;
}

int hook_formula_stagegold(int nGold, int nLevel, void *method) {
    int result = orig_formula_stagegold(nGold, nLevel, method);
    if (menu::get_toggle(TOGGLE_GOLD_MULTIPLY))
        result *= get_multiplier(0);
    return result;
}

// ============================================================================
// CRYSTAL HOOKS
// ============================================================================

// iDataCenter.AddCrystal(int nCrystal) — RVA: 0xA95EDC
typedef void (*iDataCenter_AddCrystal_t)(void *self, int nCrystal, void *method);
static iDataCenter_AddCrystal_t orig_iDataCenter_AddCrystal = nullptr;

void hook_iDataCenter_AddCrystal(void *self, int nCrystal, void *method) {
    if (menu::get_toggle(TOGGLE_CRYSTAL_MULTIPLY)) {
        int mult = get_multiplier(0);
        LOGI("iDataCenter crystal: %d -> %d (x%d)", nCrystal, nCrystal * mult, mult);
        nCrystal *= mult;
    }
    orig_iDataCenter_AddCrystal(self, nCrystal, method);
}

// iGameState.AddCrystal(int nCrystal) — RVA: 0xACAFF8
typedef void (*iGameState_AddCrystal_t)(void *self, int nCrystal, void *method);
static iGameState_AddCrystal_t orig_iGameState_AddCrystal = nullptr;

void hook_iGameState_AddCrystal(void *self, int nCrystal, void *method) {
    if (menu::get_toggle(TOGGLE_CRYSTAL_MULTIPLY)) {
        int mult = get_multiplier(0);
        LOGI("iGameState crystal: %d -> %d (x%d)", nCrystal, nCrystal * mult, mult);
        nCrystal *= mult;
    }
    orig_iGameState_AddCrystal(self, nCrystal, method);
}

// ============================================================================
// EXP HOOKS
// ============================================================================

// CCharUser.AddExp(SafeInteger nExp) — RVA: 0xA4F5A8
typedef void (*CCharUser_AddExp_t)(void *self, void *nExp, void *method);
static CCharUser_AddExp_t orig_CCharUser_AddExp = nullptr;

void hook_CCharUser_AddExp(void *self, void *nExp, void *method) {
    if (menu::get_toggle(TOGGLE_EXP_MULTIPLY) && nExp && SafeInteger_Get && SafeInteger_Set) {
        int orig = SafeInteger_Get(nExp, nullptr);
        int mult = get_multiplier(0);
        SafeInteger_Set(nExp, orig * mult, nullptr);
        LOGI("Exp: %d -> %d (x%d)", orig, orig * mult, mult);
    }
    orig_CCharUser_AddExp(self, nExp, method);
}

// MyUtils.formula_monsterexp(int nExp, int nLevel) — RVA: 0xBCB9A0
// MyUtils.formula_stageexp(int nExp, int nLevel) — RVA: 0xBCBC90
static formula_t orig_formula_monsterexp = nullptr;
static formula_t orig_formula_stageexp = nullptr;

int hook_formula_monsterexp(int nExp, int nLevel, void *method) {
    int result = orig_formula_monsterexp(nExp, nLevel, method);
    if (menu::get_toggle(TOGGLE_EXP_MULTIPLY))
        result *= get_multiplier(0);
    return result;
}

int hook_formula_stageexp(int nExp, int nLevel, void *method) {
    int result = orig_formula_stageexp(nExp, nLevel, method);
    if (menu::get_toggle(TOGGLE_EXP_MULTIPLY))
        result *= get_multiplier(0);
    return result;
}

// ============================================================================
// GOD MODE
// ============================================================================

// CCharUser.OnHit(float, CWeaponInfoLevel, string) — RVA: 0xA4F3D0
typedef bool (*OnHit_t)(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method);
static OnHit_t orig_UserOnHit = nullptr;

bool hook_UserOnHit(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method) {
    if (menu::get_toggle(TOGGLE_GOD_MODE))
        return false;
    return orig_UserOnHit(self, fDmg, pWeaponLvlInfo, sBodyPart, method);
}

// ============================================================================
// ONE-HIT KILL
// ============================================================================

// CCharMob.OnHit(float, CWeaponInfoLevel, string) — RVA: 0x941EA8
static OnHit_t orig_MobOnHit = nullptr;

bool hook_MobOnHit(void *self, float fDmg, void *pWeaponLvlInfo, void *sBodyPart, void *method) {
    if (menu::get_toggle(TOGGLE_ONE_HIT))
        fDmg = 999999.0f;
    return orig_MobOnHit(self, fDmg, pWeaponLvlInfo, sBodyPart, method);
}

// ============================================================================
// UNLIMITED AMMO
// ============================================================================

// CWeaponBase.ConsumeBullet(CCharPlayer player) — RVA: 0xB94E24
typedef void (*ConsumeBullet_t)(void *self, void *player, void *method);
static ConsumeBullet_t orig_ConsumeBullet = nullptr;

void hook_ConsumeBullet(void *self, void *player, void *method) {
    if (menu::get_toggle(TOGGLE_UNLIMITED_AMMO))
        return; // don't consume bullets
    orig_ConsumeBullet(self, player, method);
}

// CWeaponBase.get_IsBulletEmpty() — RVA: 0xB93B30
typedef bool (*IsBulletEmpty_t)(void *self, void *method);
static IsBulletEmpty_t orig_IsBulletEmpty = nullptr;

bool hook_IsBulletEmpty(void *self, void *method) {
    if (menu::get_toggle(TOGGLE_UNLIMITED_AMMO))
        return false;
    return orig_IsBulletEmpty(self, method);
}

// ============================================================================
// DAMAGE MULTIPLIER
// ============================================================================

// CCharPlayer.CalcWeaponDamage(CWeaponInfoLevel) — RVA: 0x950A40
typedef float (*CalcWeaponDamage_t)(void *self, void *weaponLvlInfo, void *method);
static CalcWeaponDamage_t orig_CalcWeaponDamage = nullptr;

float hook_CalcWeaponDamage(void *self, void *weaponLvlInfo, void *method) {
    float dmg = orig_CalcWeaponDamage(self, weaponLvlInfo, method);
    if (menu::get_toggle(TOGGLE_DAMAGE_MULTIPLY)) {
        int mult = get_multiplier(1);
        dmg *= (float)mult;
    }
    return dmg;
}

// ============================================================================
// HOOK INSTALLATION
// ============================================================================

static void resolve_safe_integer(uintptr_t base) {
    SafeInteger_Get = reinterpret_cast<SafeInteger_Get_t>(base + 0x141EBA0);
    SafeInteger_Set = reinterpret_cast<SafeInteger_Set_t>(base + 0x141EB44);
    LOGI("SafeInteger.Get: %p, SafeInteger.Set: %p",
         (void *)SafeInteger_Get, (void *)SafeInteger_Set);
}

#define HOOK(offset, hook_fn, orig_fn, label) \
    hook::hook_addr(base + offset, (void *)hook_fn, (void **)&orig_fn); \
    LOGI("Hooked " label " @ 0x%X", offset);

void install_hooks() {
    uintptr_t base = il2cpp::get_base_address();
    resolve_safe_integer(base);

    // --- Gold ---
    HOOK(0x12BB194, hook_CCharUser_AddGold,   orig_CCharUser_AddGold,   "CCharUser.AddGold");
    HOOK(0x12F30B4, hook_iDataCenter_AddGold,  orig_iDataCenter_AddGold, "iDataCenter.AddGold");
    HOOK(0x13E626C, hook_formula_monstergold,  orig_formula_monstergold, "formula_monstergold");
    HOOK(0x13E649C, hook_formula_stagegold,    orig_formula_stagegold,   "formula_stagegold");

    // --- Crystal ---
    HOOK(0x12F30FC, hook_iDataCenter_AddCrystal,  orig_iDataCenter_AddCrystal,  "iDataCenter.AddCrystal");
    HOOK(0x131D41C, hook_iGameState_AddCrystal,   orig_iGameState_AddCrystal,   "iGameState.AddCrystal");

    // --- EXP ---
    HOOK(0x12BA99C, hook_CCharUser_AddExp,    orig_CCharUser_AddExp,    "CCharUser.AddExp");
    HOOK(0x13E6370, hook_formula_monsterexp,  orig_formula_monsterexp,  "formula_monsterexp");
    HOOK(0x13E65A0, hook_formula_stageexp,    orig_formula_stageexp,    "formula_stageexp");

    // --- God Mode ---
    HOOK(0x12BA818, hook_UserOnHit,  orig_UserOnHit,  "CCharUser.OnHit");

    // --- One-Hit Kill ---
    HOOK(0x11E80E0, hook_MobOnHit,   orig_MobOnHit,   "CCharMob.OnHit");

    // --- Unlimited Ammo ---
    HOOK(0x13BC39C, hook_ConsumeBullet,  orig_ConsumeBullet,  "CWeaponBase.ConsumeBullet");
    HOOK(0x13BB4F0, hook_IsBulletEmpty,  orig_IsBulletEmpty,  "CWeaponBase.IsBulletEmpty");

    // --- Damage Multiplier ---
    HOOK(0x11F3D18, hook_CalcWeaponDamage, orig_CalcWeaponDamage, "CCharPlayer.CalcWeaponDamage");

    LOGI("=== All hooks installed ===");
    LOGI("Toggle 0: Gold Multiply     (slider 0 = multiplier, default 10x)");
    LOGI("Toggle 1: Crystal Multiply  (slider 0 = multiplier, default 10x)");
    LOGI("Toggle 2: God Mode          (no damage taken)");
    LOGI("Toggle 3: One-Hit Kill      (999999 damage to mobs)");
    LOGI("Toggle 4: Unlimited Ammo    (no bullet consumption)");
    LOGI("Toggle 5: EXP Multiply      (slider 0 = multiplier, default 10x)");
    LOGI("Toggle 6: Damage Multiply   (slider 1 = multiplier, default 10x)");
}
