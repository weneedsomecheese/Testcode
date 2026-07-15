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
    uintptr_t page_end = (reinterpret_cast<uintptr_t>(addr) + len + page_size - 1) & ~(page_size - 1);
    return mprotect(reinterpret_cast<void *>(page_start), page_end - page_start, prot) == 0;
}

static void flush_cache(void *addr, size_t len) {
    uintptr_t start = reinterpret_cast<uintptr_t>(addr);
    uintptr_t end = start + len;
    uint64_t ctr;
    asm volatile("mrs %0, ctr_el0" : "=r"(ctr));
    size_t dcache_line = 4 << ((ctr >> 16) & 0xF);
    size_t icache_line = 4 << ((ctr >> 0) & 0xF);
    for (uintptr_t p = start & ~(dcache_line - 1); p < end; p += dcache_line)
        asm volatile("dc cvau, %0" :: "r"(p));
    asm volatile("dsb ish");
    for (uintptr_t p = start & ~(icache_line - 1); p < end; p += icache_line)
        asm volatile("ic ivau, %0" :: "r"(p));
    asm volatile("dsb ish");
    asm volatile("isb");
}

// ============================================================================
// ARM64 instruction relocation for PC-relative instructions
// ============================================================================

static bool is_adrp(uint32_t insn) { return (insn & 0x9F000000) == 0x90000000; }
static bool is_adr(uint32_t insn)  { return (insn & 0x9F000000) == 0x10000000; }
static bool is_b_bl(uint32_t insn) { return (insn & 0x7C000000) == 0x14000000; }
static bool is_bcond(uint32_t insn){ return (insn & 0xFF000010) == 0x54000000; }
static bool is_cbz(uint32_t insn)  { return (insn & 0x7E000000) == 0x34000000; }
static bool is_tbz(uint32_t insn)  { return (insn & 0x7E000000) == 0x36000000; }
static bool is_ldr_lit(uint32_t insn) { return (insn & 0x3B000000) == 0x18000000; }

static uint32_t relocate_insn(uint32_t insn, uintptr_t from_pc, uintptr_t to_pc) {
    if (is_adrp(insn)) {
        int rd = insn & 0x1F;
        int immlo = (insn >> 29) & 0x3;
        int immhi = (insn >> 5) & 0x7FFFF;
        int32_t imm21 = (immhi << 2) | immlo;
        if (imm21 & (1 << 20)) imm21 |= (int32_t)0xFFE00000;
        int64_t target_page = (int64_t)(from_pc & ~0xFFFULL) + ((int64_t)imm21 << 12);
        int64_t new_imm = (target_page - (int64_t)(to_pc & ~0xFFFULL)) >> 12;
        if (new_imm >= -(1 << 20) && new_imm < (1 << 20)) {
            int nlo = (int)(new_imm & 0x3);
            int nhi = (int)((new_imm >> 2) & 0x7FFFF);
            return 0x90000000 | ((uint32_t)nlo << 29) | ((uint32_t)nhi << 5) | rd;
        }
    }

    if (is_adr(insn)) {
        int rd = insn & 0x1F;
        int immlo = (insn >> 29) & 0x3;
        int immhi = (insn >> 5) & 0x7FFFF;
        int32_t imm21 = (immhi << 2) | immlo;
        if (imm21 & (1 << 20)) imm21 |= (int32_t)0xFFE00000;
        int64_t target = (int64_t)from_pc + imm21;
        int64_t new_imm = target - (int64_t)to_pc;
        if (new_imm >= -(1 << 20) && new_imm < (1 << 20)) {
            int nlo = (int)(new_imm & 0x3);
            int nhi = (int)((new_imm >> 2) & 0x7FFFF);
            return 0x10000000 | ((uint32_t)nlo << 29) | ((uint32_t)nhi << 5) | rd;
        }
    }

    if (is_b_bl(insn)) {
        int is_bl = (insn >> 31) & 1;
        int32_t imm26 = insn & 0x3FFFFFF;
        if (imm26 & (1 << 25)) imm26 |= (int32_t)0xFC000000;
        int64_t target = (int64_t)from_pc + ((int64_t)imm26 << 2);
        int64_t new_off = (target - (int64_t)to_pc) >> 2;
        if (new_off >= -(1 << 25) && new_off < (1 << 25)) {
            return ((uint32_t)is_bl << 31) | 0x14000000 | ((uint32_t)new_off & 0x3FFFFFF);
        }
    }

    if (is_bcond(insn)) {
        int32_t imm19 = (insn >> 5) & 0x7FFFF;
        if (imm19 & (1 << 18)) imm19 |= (int32_t)0xFFF80000;
        int64_t target = (int64_t)from_pc + ((int64_t)imm19 << 2);
        int64_t new_off = (target - (int64_t)to_pc) >> 2;
        if (new_off >= -(1 << 18) && new_off < (1 << 18)) {
            return (insn & ~(0x7FFFFu << 5)) | (((uint32_t)new_off & 0x7FFFF) << 5);
        }
    }

    if (is_cbz(insn)) {
        int32_t imm19 = (insn >> 5) & 0x7FFFF;
        if (imm19 & (1 << 18)) imm19 |= (int32_t)0xFFF80000;
        int64_t target = (int64_t)from_pc + ((int64_t)imm19 << 2);
        int64_t new_off = (target - (int64_t)to_pc) >> 2;
        if (new_off >= -(1 << 18) && new_off < (1 << 18)) {
            return (insn & ~(0x7FFFFu << 5)) | (((uint32_t)new_off & 0x7FFFF) << 5);
        }
    }

    if (is_tbz(insn)) {
        int32_t imm14 = (insn >> 5) & 0x3FFF;
        if (imm14 & (1 << 13)) imm14 |= (int32_t)0xFFFFC000;
        int64_t target = (int64_t)from_pc + ((int64_t)imm14 << 2);
        int64_t new_off = (target - (int64_t)to_pc) >> 2;
        if (new_off >= -(1 << 13) && new_off < (1 << 13)) {
            return (insn & ~(0x3FFFu << 5)) | (((uint32_t)new_off & 0x3FFF) << 5);
        }
    }

    if (is_ldr_lit(insn)) {
        int32_t imm19 = (insn >> 5) & 0x7FFFF;
        if (imm19 & (1 << 18)) imm19 |= (int32_t)0xFFF80000;
        int64_t target = (int64_t)from_pc + ((int64_t)imm19 << 2);
        int64_t new_off = (target - (int64_t)to_pc) >> 2;
        if (new_off >= -(1 << 18) && new_off < (1 << 18)) {
            return (insn & ~(0x7FFFFu << 5)) | (((uint32_t)new_off & 0x7FFFF) << 5);
        }
    }

    return insn;
}

// ============================================================================

bool init() {
    LOGI("Hook engine initialized (ARM64 inline hook)");
    return true;
}

bool hook_function(void *target, void *replacement, void **original) {
    if (!target || !replacement || hook_count >= MAX_HOOKS) return false;

    uintptr_t target_addr = reinterpret_cast<uintptr_t>(target);

    HookEntry *entry = &hooks[hook_count];
    entry->target = target;
    entry->hook_size = 16;

    memcpy(entry->original_bytes, target, entry->hook_size);

    uint8_t *trampoline = static_cast<uint8_t *>(
        mmap(0, 4096, PROT_READ | PROT_WRITE,
             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (trampoline == MAP_FAILED) {
        LOGE("Failed to allocate trampoline");
        return false;
    }

    for (int i = 0; i < 4; i++) {
        uint32_t insn;
        memcpy(&insn, &entry->original_bytes[i * 4], 4);
        uintptr_t old_pc = target_addr + i * 4;
        uintptr_t new_pc = reinterpret_cast<uintptr_t>(trampoline) + i * 4;
        uint32_t relocated = relocate_insn(insn, old_pc, new_pc);
        memcpy(&trampoline[i * 4], &relocated, 4);
    }

    uint32_t ldr_x17 = 0x58000051;
    uint32_t br_x17  = 0xD61F0220;
    uintptr_t continue_addr = target_addr + entry->hook_size;

    memcpy(&trampoline[entry->hook_size], &ldr_x17, 4);
    memcpy(&trampoline[entry->hook_size + 4], &br_x17, 4);
    memcpy(&trampoline[entry->hook_size + 8], &continue_addr, 8);

    mprotect(trampoline, 4096, PROT_READ | PROT_EXEC);
    flush_cache(trampoline, entry->hook_size + 16);

    entry->trampoline = trampoline;
    if (original) *original = trampoline;

    if (!set_mem_perms(target, entry->hook_size, PROT_READ | PROT_WRITE)) {
        LOGE("Failed to set memory permissions at %p", target);
        munmap(trampoline, 4096);
        return false;
    }

    uint8_t *patch = static_cast<uint8_t *>(target);
    uintptr_t repl_addr = reinterpret_cast<uintptr_t>(replacement);
    memcpy(&patch[0], &ldr_x17, 4);
    memcpy(&patch[4], &br_x17, 4);
    memcpy(&patch[8], &repl_addr, 8);

    set_mem_perms(target, entry->hook_size, PROT_READ | PROT_EXEC);
    flush_cache(target, entry->hook_size);

    hook_count++;
    LOGI("Hooked %p -> %p (trampoline: %p)", target, replacement, (void *)trampoline);
    return true;
}

bool unhook_function(void *target) {
    for (int i = 0; i < hook_count; i++) {
        if (hooks[i].target == target) {
            set_mem_perms(target, hooks[i].hook_size, PROT_READ | PROT_WRITE);
            memcpy(target, hooks[i].original_bytes, hooks[i].hook_size);
            flush_cache(target, hooks[i].hook_size);
            set_mem_perms(target, hooks[i].hook_size, PROT_READ | PROT_EXEC);

            if (hooks[i].trampoline)
                munmap(hooks[i].trampoline, 4096);

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
