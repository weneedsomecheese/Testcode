#pragma once

#include "android_compat.h"

namespace hook {

bool init();

bool hook_function(void *target, void *replacement, void **original);
bool unhook_function(void *target);

inline bool hook_addr(uintptr_t addr, void *replacement, void **original) {
    return hook_function(reinterpret_cast<void *>(addr), replacement, original);
}

template <typename T>
bool hook_method(void *target, T replacement, T *original) {
    return hook_function(target, reinterpret_cast<void *>(replacement),
                         reinterpret_cast<void **>(original));
}

} // namespace hook
