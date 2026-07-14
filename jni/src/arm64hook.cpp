#include "hook.h"
#include "log.h"

namespace hook {

struct HookEntry {
    void *target;
    void *trampoline;
    uint8_t original_bytes[16];
    size_t hook_size;
};

static constexpr int MAX_HOOKS = 64;
static HookEntry hooks[MAX_HOOKS];
static int hook_count = 0;

static bool set_mem_perms(void *addr, size_t len, int prot) {
    uintptr_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = reinterpret_cast<uintptr_t>(addr) & ~(page_size - 1);
    return mprotect(reinterpret_cast<void *>(page_start), len + (reinterpret_cast<uintptr_t>(addr) - page_start), prot) == 0;
}

static void flush_cache(void *addr, size_t len) {
    __builtin___clear_cache(static_cast<char *>(addr), static_cast<char *>(addr) + len);
}

bool init() {
    LOGI("Hook engine initialized (ARM64 inline hook)");
    return true;
}

// ARM64 hook: overwrites 16 bytes at target with:
//   LDR X17, #8      (0x58000051) — load address from PC+8
//   BR  X17           (0xD61F0220) — branch to it
//   <64-bit address>  — replacement function pointer
//
// Trampoline: copies original 16 bytes, then jumps back:
//   <original 16 bytes>
//   LDR X17, #8
//   BR  X17
//   <64-bit continue address>

bool hook_function(void *target, void *replacement, void **original) {
    if (!target || !replacement || hook_count >= MAX_HOOKS) return false;

    uintptr_t target_addr = reinterpret_cast<uintptr_t>(target);

    HookEntry *entry = &hooks[hook_count];
    entry->target = target;
    entry->hook_size = 16;

    memcpy(entry->original_bytes, target, entry->hook_size);

    // Allocate trampoline: 16 (original) + 4 (LDR) + 4 (BR) + 8 (addr) = 32 bytes
    uint8_t *trampoline = static_cast<uint8_t *>(
        mmap(0, 64, PROT_READ | PROT_WRITE | PROT_EXEC,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (trampoline == MAP_FAILED) {
        LOGE("Failed to allocate trampoline");
        return false;
    }

    // Copy original instructions to trampoline
    memcpy(trampoline, entry->original_bytes, entry->hook_size);

    // Add jump back: LDR X17, #8; BR X17; <continue_addr>
    uint32_t ldr_x17 = 0x58000051;
    uint32_t br_x17  = 0xD61F0220;
    uintptr_t continue_addr = target_addr + entry->hook_size;

    memcpy(&trampoline[entry->hook_size], &ldr_x17, 4);
    memcpy(&trampoline[entry->hook_size + 4], &br_x17, 4);
    memcpy(&trampoline[entry->hook_size + 8], &continue_addr, 8);
    flush_cache(trampoline, entry->hook_size + 16);

    entry->trampoline = trampoline;
    if (original) *original = trampoline;

    // Patch target: LDR X17, #8; BR X17; <replacement_addr>
    if (!set_mem_perms(target, entry->hook_size, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        LOGE("Failed to set memory permissions at %p", target);
        munmap(trampoline, 64);
        return false;
    }

    uint8_t *patch = static_cast<uint8_t *>(target);
    uintptr_t repl_addr = reinterpret_cast<uintptr_t>(replacement);
    memcpy(&patch[0], &ldr_x17, 4);
    memcpy(&patch[4], &br_x17, 4);
    memcpy(&patch[8], &repl_addr, 8);
    flush_cache(target, entry->hook_size);

    set_mem_perms(target, entry->hook_size, PROT_READ | PROT_EXEC);

    hook_count++;
    LOGI("Hooked %p -> %p (trampoline: %p)", target, replacement, (void *)trampoline);
    return true;
}

bool unhook_function(void *target) {
    for (int i = 0; i < hook_count; i++) {
        if (hooks[i].target == target) {
            set_mem_perms(target, hooks[i].hook_size, PROT_READ | PROT_WRITE | PROT_EXEC);
            memcpy(target, hooks[i].original_bytes, hooks[i].hook_size);
            flush_cache(target, hooks[i].hook_size);
            set_mem_perms(target, hooks[i].hook_size, PROT_READ | PROT_EXEC);

            if (hooks[i].trampoline)
                munmap(hooks[i].trampoline, 64);

            for (int j = i; j < hook_count - 1; j++)
                hooks[j] = hooks[j + 1];
            hook_count--;

            LOGI("Unhooked %p", target);
            return true;
        }
    }
    return false;
}

} // namespace hook
