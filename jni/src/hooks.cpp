#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

// ============================================================================
// HOOK DEFINITIONS
//
// To add hooks for your game:
// 1. Use Il2CppDumper to get the dump.cs from your game's libil2cpp.so
// 2. Find the methods you want to hook in dump.cs
// 3. Get the method's RVA (offset) from the dump
// 4. Define your hook function and original function pointer
// 5. Register it in install_hooks()
//
// Two approaches to hooking:
//   A) By offset (faster, but breaks on game updates):
//      hook::hook_addr(il2cpp::get_base_address() + OFFSET, hook_fn, &orig_fn);
//
//   B) By class/method name (survives updates if method signature unchanged):
//      auto *klass = il2cpp::find_class("", "PlayerStats");
//      auto *method = il2cpp::class_get_method_from_name(klass, "TakeDamage", 1);
//      hook::hook_function(il2cpp::get_method_pointer(method), hook_fn, &orig_fn);
// ============================================================================

// --- Example: Hook a method by offset ---
// Replace 0xABCDEF with the actual RVA from your dump.cs

// Toggle indices for the mod menu
enum Toggle {
    TOGGLE_GOD_MODE = 0,
    TOGGLE_ONE_HIT = 1,
    TOGGLE_UNLIMITED_AMMO = 2,
    TOGGLE_SPEED_HACK = 3,
};

// Example: Player.TakeDamage(float amount)
typedef void (*TakeDamage_t)(void *self, float amount, void *method);
static TakeDamage_t orig_TakeDamage = nullptr;

void hook_TakeDamage(void *self, float amount, void *method) {
    if (menu::get_toggle(TOGGLE_GOD_MODE)) {
        return; // skip damage
    }
    orig_TakeDamage(self, amount, method);
}

// Example: Enemy.get_Health()
typedef float (*GetHealth_t)(void *self, void *method);
static GetHealth_t orig_GetHealth = nullptr;

float hook_GetHealth(void *self, void *method) {
    if (menu::get_toggle(TOGGLE_ONE_HIT)) {
        return 0.0f; // enemies always at 0 HP
    }
    return orig_GetHealth(self, method);
}

// Example: Weapon.get_AmmoCount()
typedef int (*GetAmmoCount_t)(void *self, void *method);
static GetAmmoCount_t orig_GetAmmoCount = nullptr;

int hook_GetAmmoCount(void *self, void *method) {
    if (menu::get_toggle(TOGGLE_UNLIMITED_AMMO)) {
        return 999;
    }
    return orig_GetAmmoCount(self, method);
}

// Example: Player.get_MoveSpeed()
typedef float (*GetMoveSpeed_t)(void *self, void *method);
static GetMoveSpeed_t orig_GetMoveSpeed = nullptr;

float hook_GetMoveSpeed(void *self, void *method) {
    float speed = orig_GetMoveSpeed(self, method);
    if (menu::get_toggle(TOGGLE_SPEED_HACK)) {
        float multiplier = 1.0f + (menu::get_slider(0) / 10.0f);
        return speed * multiplier;
    }
    return speed;
}

// ============================================================================
// HOOK INSTALLATION
// ============================================================================

void install_hooks_by_offset() {
    uintptr_t base = il2cpp::get_base_address();

    // Replace these with real offsets from your dump.cs
    // Format: hook::hook_addr(base + OFFSET, (void *)hook_fn, (void **)&orig_fn);
    //
    // Example (uncomment and replace offsets):
    // hook::hook_addr(base + 0x123456, (void *)hook_TakeDamage, (void **)&orig_TakeDamage);
    // hook::hook_addr(base + 0x234567, (void *)hook_GetHealth, (void **)&orig_GetHealth);
    // hook::hook_addr(base + 0x345678, (void *)hook_GetAmmoCount, (void **)&orig_GetAmmoCount);
    // hook::hook_addr(base + 0x456789, (void *)hook_GetMoveSpeed, (void **)&orig_GetMoveSpeed);

    LOGI("Offset-based hooks installed (update offsets for your game)");
}

void install_hooks_by_name() {
    // Approach B: Find methods by class and method name
    // This is more robust across game updates

    // Example (uncomment and update class/method names from your dump.cs):
    //
    // auto *player_class = il2cpp::find_class("", "Player");
    // if (player_class) {
    //     auto *take_damage = il2cpp::class_get_method_from_name(player_class, "TakeDamage", 1);
    //     if (take_damage) {
    //         hook::hook_method(il2cpp::get_method_pointer(take_damage),
    //                           hook_TakeDamage, &orig_TakeDamage);
    //     }
    //
    //     auto *get_speed = il2cpp::class_get_method_from_name(player_class, "get_MoveSpeed", 0);
    //     if (get_speed) {
    //         hook::hook_method(il2cpp::get_method_pointer(get_speed),
    //                           hook_GetMoveSpeed, &orig_GetMoveSpeed);
    //     }
    // }
    //
    // auto *enemy_class = il2cpp::find_class("", "Enemy");
    // if (enemy_class) {
    //     auto *get_health = il2cpp::class_get_method_from_name(enemy_class, "get_Health", 0);
    //     if (get_health) {
    //         hook::hook_method(il2cpp::get_method_pointer(get_health),
    //                           hook_GetHealth, &orig_GetHealth);
    //     }
    // }

    LOGI("Name-based hooks installed (update class/method names for your game)");
}

void install_hooks() {
    // Choose one approach or mix both:
    install_hooks_by_offset();
    // install_hooks_by_name();
}
