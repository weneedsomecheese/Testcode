#include "hook.h"
#include "log.h"

#include <sys/mman.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace hook {

struct HookEntry {
    void *target;
    void *trampoline;
    uint8_t original_bytes[16];
    size_t hook_size;
};

static std::vector<HookEntry> hooks;

static bool set_mem_perms(void *addr, size_t len, int prot) {
    uintptr_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = reinterpret_cast<uintptr_t>(addr) & ~(page_size - 1);
    return mprotect(reinterpret_cast<void *>(page_start), len + (reinterpret_cast<uintptr_t>(addr) - page_start), prot) == 0;
}

static void flush_cache(void *addr, size_t len) {
    __builtin___clear_cache(static_cast<char *>(addr), static_cast<char *>(addr) + len);
}

bool init() {
    LOGI("Hook engine initialized (Thumb inline hook)");
    return true;
}

bool hook_function(void *target, void *replacement, void **original) {
    if (!target || !replacement) return false;

    uintptr_t target_addr = reinterpret_cast<uintptr_t>(target);
    bool is_thumb = target_addr & 1;
    void *aligned_target = reinterpret_cast<void *>(target_addr & ~1u);

    HookEntry entry = {};
    entry.target = aligned_target;

    if (is_thumb) {
        // Thumb mode: LDR PC, [PC, #0]; .word replacement_addr
        // T1: 0xF000F8DF = ldr.w pc, [pc]
        entry.hook_size = 8;
        memcpy(entry.original_bytes, aligned_target, entry.hook_size);

        uint8_t *trampoline = static_cast<uint8_t *>(
            mmap(nullptr, 32, PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (trampoline == MAP_FAILED) {
            LOGE("Failed to allocate trampoline");
            return false;
        }

        memcpy(trampoline, entry.original_bytes, entry.hook_size);
        // LDR.W PC, [PC, #0]
        trampoline[entry.hook_size + 0] = 0xDF;
        trampoline[entry.hook_size + 1] = 0xF8;
        trampoline[entry.hook_size + 2] = 0x00;
        trampoline[entry.hook_size + 3] = 0xF0;
        uintptr_t continue_addr = target_addr + entry.hook_size;
        memcpy(&trampoline[entry.hook_size + 4], &continue_addr, 4);
        flush_cache(trampoline, entry.hook_size + 8);

        entry.trampoline = trampoline;
        if (original) *original = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(trampoline) | 1);

        if (!set_mem_perms(aligned_target, entry.hook_size + 4, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            LOGE("Failed to set memory permissions at %p", aligned_target);
            munmap(trampoline, 32);
            return false;
        }

        uint8_t *patch = static_cast<uint8_t *>(aligned_target);
        // LDR.W PC, [PC, #0]
        patch[0] = 0xDF;
        patch[1] = 0xF8;
        patch[2] = 0x00;
        patch[3] = 0xF0;
        uintptr_t repl_addr = reinterpret_cast<uintptr_t>(replacement);
        memcpy(&patch[4], &repl_addr, 4);

        flush_cache(aligned_target, entry.hook_size);

    } else {
        // ARM mode: LDR PC, [PC, #-4]; .word replacement_addr
        entry.hook_size = 8;
        memcpy(entry.original_bytes, aligned_target, entry.hook_size);

        uint8_t *trampoline = static_cast<uint8_t *>(
            mmap(nullptr, 32, PROT_READ | PROT_WRITE | PROT_EXEC,
                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (trampoline == MAP_FAILED) {
            LOGE("Failed to allocate trampoline");
            return false;
        }

        memcpy(trampoline, entry.original_bytes, entry.hook_size);
        // LDR PC, [PC, #-4]
        uint32_t ldr_pc = 0xE51FF004;
        memcpy(&trampoline[entry.hook_size], &ldr_pc, 4);
        uintptr_t continue_addr = target_addr + entry.hook_size;
        memcpy(&trampoline[entry.hook_size + 4], &continue_addr, 4);
        flush_cache(trampoline, entry.hook_size + 8);

        entry.trampoline = trampoline;
        if (original) *original = trampoline;

        if (!set_mem_perms(aligned_target, entry.hook_size + 4, PROT_READ | PROT_WRITE | PROT_EXEC)) {
            LOGE("Failed to set memory permissions at %p", aligned_target);
            munmap(trampoline, 32);
            return false;
        }

        uint32_t *patch = static_cast<uint32_t *>(aligned_target);
        patch[0] = 0xE51FF004;  // LDR PC, [PC, #-4]
        uintptr_t repl_addr = reinterpret_cast<uintptr_t>(replacement);
        memcpy(&patch[1], &repl_addr, 4);

        flush_cache(aligned_target, entry.hook_size);
    }

    set_mem_perms(aligned_target, entry.hook_size + 4, PROT_READ | PROT_EXEC);

    hooks.push_back(entry);
    LOGI("Hooked %p -> %p (trampoline: %p)", target, replacement, entry.trampoline);
    return true;
}

bool unhook_function(void *target) {
    uintptr_t target_addr = reinterpret_cast<uintptr_t>(target);
    void *aligned_target = reinterpret_cast<void *>(target_addr & ~1u);

    for (auto it = hooks.begin(); it != hooks.end(); ++it) {
        if (it->target == aligned_target) {
            set_mem_perms(aligned_target, it->hook_size, PROT_READ | PROT_WRITE | PROT_EXEC);
            memcpy(aligned_target, it->original_bytes, it->hook_size);
            flush_cache(aligned_target, it->hook_size);
            set_mem_perms(aligned_target, it->hook_size, PROT_READ | PROT_EXEC);

            if (it->trampoline)
                munmap(it->trampoline, 32);

            hooks.erase(it);
            LOGI("Unhooked %p", target);
            return true;
        }
    }
    return false;
}

} // namespace hook
