#!/usr/bin/env python3
"""
Dino Hunter Multiplayer - IL2CPP GameAssembly.dll Patcher v2

Patches native x86-64 code in GameAssembly.dll for:
  1. God Mode        - Player cannot take damage
  2. One-Hit Kill    - All enemies die in one hit
  3. Unlimited Ammo  - Never consume bullets, never empty
  4. Max Gold        - Gold always reads as 999,999,999
  5. Max Crystal     - Crystals always read as 999,999,999
  6. 50x Hunter EXP  - Hunter level EXP multiplied by 50
  7. 50x Char EXP    - Character level EXP multiplied by 50

Usage:
  python il2cpp_patcher.py                        (looks for GameAssembly.dll in current dir)
  python il2cpp_patcher.py C:\Games\DinoHunter\GameAssembly.dll

IMPORTANT: Restore from .backup before re-patching if you previously
           applied an older version of this patcher.
"""

import struct
import shutil
import sys
import os

NEG_999999_BYTES = struct.pack('<f', -999999.0)

PATCHES = [
    {
        "name": "God Mode",
        "desc": "CCharUser.OnHit -> return false (player never takes damage)",
        "offset": 0x39F450,
        "bytes": bytes([
            0x31, 0xC0,  # xor eax, eax   ; eax = 0 (false)
            0xC3,        # ret
        ]),
    },
    {
        "name": "One-Hit Kill",
        "desc": "CCharBase.OnHit -> subtract 999999 from HP, return true",
        "offset": 0x2A25D0,
        "bytes": bytes([
            # movss xmm0, dword ptr [rcx+0x124]   ; load this->m_fHP
            0xF3, 0x0F, 0x10, 0x81, 0x24, 0x01, 0x00, 0x00,
            # mov eax, <-999999.0f as uint32>      ; load float constant
            0xB8, NEG_999999_BYTES[0], NEG_999999_BYTES[1],
                  NEG_999999_BYTES[2], NEG_999999_BYTES[3],
            # movd xmm1, eax                       ; move to SSE register
            0x66, 0x0F, 0x6E, 0xC8,
            # addss xmm0, xmm1                     ; HP += (-999999) = HP - 999999
            0xF3, 0x0F, 0x58, 0xC1,
            # movss dword ptr [rcx+0x124], xmm0    ; store this->m_fHP
            0xF3, 0x0F, 0x11, 0x81, 0x24, 0x01, 0x00, 0x00,
            # mov eax, 1                            ; return true (hit landed)
            0xB8, 0x01, 0x00, 0x00, 0x00,
            # ret
            0xC3,
        ]),
    },
    {
        "name": "Unlimited Ammo (no consume)",
        "desc": "CWeaponBase.ConsumeBullet -> return immediately (bullets never decrease)",
        "offset": 0x4AB460,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    {
        "name": "Unlimited Ammo (never empty)",
        "desc": "CWeaponBase.get_IsBulletEmpty -> return false (gun never reports empty)",
        "offset": 0x4AC880,
        "bytes": bytes([
            0x31, 0xC0,  # xor eax, eax   ; eax = 0 (false)
            0xC3,        # ret
        ]),
    },
    {
        "name": "Max Gold (999,999,999)",
        "desc": "iDataCenter.get_Gold -> always return 999999999",
        "offset": 0x3DEFC0,
        "bytes": bytes([
            # mov eax, 999999999 (0x3B9AC9FF)
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,
            # ret
            0xC3,
        ]),
    },
    {
        "name": "Max Crystal (999,999,999)",
        "desc": "iDataCenter.get_Crystal -> always return 999999999",
        "offset": 0x3DEF20,
        "bytes": bytes([
            # mov eax, 999999999 (0x3B9AC9FF)
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,
            # ret
            0xC3,
        ]),
    },
    #
    # ── 50x EXP multiplier ──
    #
    {
        "name": "50x Hunter EXP",
        "desc": "iDataCenter.AddHunterExp -> multiply exp by 50, add to HunterExp + HunterExpTotal",
        "offset": 0x3CFBF0,
        "bytes": bytes([
            0x53,                                          # push rbx
            0x56,                                          # push rsi
            0x48, 0x83, 0xEC, 0x28,                       # sub rsp, 0x28
            0x48, 0x8B, 0xD9,                             # mov rbx, rcx          ; this
            0x6B, 0xF2, 0x32,                             # imul esi, edx, 50     ; esi = nHunterExp * 50
            # m_nHunterExp += esi
            0x48, 0x8B, 0x8B, 0xD8, 0x01, 0x00, 0x00,   # mov rcx, [rbx+0x1D8] ; m_nHunterExp
            0xE8, 0xA8, 0xF9, 0x14, 0x00,                # call SafeInteger.Get
            0x01, 0xF0,                                    # add eax, esi
            0x48, 0x8B, 0x8B, 0xD8, 0x01, 0x00, 0x00,   # mov rcx, [rbx+0x1D8]
            0x8B, 0xD0,                                    # mov edx, eax
            0xE8, 0x68, 0xFA, 0x14, 0x00,                # call SafeInteger.Set
            # m_nHunterExpTotal += esi
            0x48, 0x8B, 0x8B, 0xE0, 0x01, 0x00, 0x00,   # mov rcx, [rbx+0x1E0] ; m_nHunterExpTotal
            0xE8, 0x8C, 0xF9, 0x14, 0x00,                # call SafeInteger.Get
            0x01, 0xF0,                                    # add eax, esi
            0x48, 0x8B, 0x8B, 0xE0, 0x01, 0x00, 0x00,   # mov rcx, [rbx+0x1E0]
            0x8B, 0xD0,                                    # mov edx, eax
            0xE8, 0x4C, 0xFA, 0x14, 0x00,                # call SafeInteger.Set
            0x48, 0x83, 0xC4, 0x28,                       # add rsp, 0x28
            0x5E,                                          # pop rsi
            0x5B,                                          # pop rbx
            0xC3,                                          # ret
        ]),
    },
    {
        "name": "50x Character EXP",
        "desc": "CCharUser.AddExp -> multiply exp by 50, add to m_nExp, call LevelUp",
        "offset": 0x39D3D0,
        "bytes": bytes([
            0x53,                                          # push rbx
            0x56,                                          # push rsi
            0x57,                                          # push rdi
            0x48, 0x83, 0xEC, 0x20,                       # sub rsp, 0x20
            0x48, 0x8B, 0xD9,                             # mov rbx, rcx          ; this
            0x48, 0x8B, 0xF2,                             # mov rsi, rdx          ; nExp (SafeInteger)
            # get the exp value from parameter
            0x48, 0x8B, 0xCE,                             # mov rcx, rsi
            0xE8, 0xCB, 0x21, 0x18, 0x00,                # call SafeInteger.Get
            0x6B, 0xF8, 0x32,                             # imul edi, eax, 50     ; edi = exp * 50
            # m_nExp += edi
            0x48, 0x8B, 0x8B, 0x58, 0x03, 0x00, 0x00,   # mov rcx, [rbx+0x358] ; m_nExp
            0xE8, 0xBC, 0x21, 0x18, 0x00,                # call SafeInteger.Get
            0x01, 0xF8,                                    # add eax, edi
            0x48, 0x8B, 0x8B, 0x58, 0x03, 0x00, 0x00,   # mov rcx, [rbx+0x358]
            0x8B, 0xD0,                                    # mov edx, eax
            0xE8, 0x7C, 0x22, 0x18, 0x00,                # call SafeInteger.Set
            # LevelUp(this, ref m_nExp, ref m_nLevel)
            0x48, 0x8B, 0xCB,                             # mov rcx, rbx
            0x48, 0x8D, 0x93, 0x58, 0x03, 0x00, 0x00,   # lea rdx, [rbx+0x358] ; ref m_nExp
            0x4C, 0x8D, 0x83, 0x50, 0x03, 0x00, 0x00,   # lea r8, [rbx+0x350]  ; ref m_nLevel
            0xE8, 0x26, 0x17, 0x00, 0x00,                # call LevelUp
            0x48, 0x83, 0xC4, 0x20,                       # add rsp, 0x20
            0x5F,                                          # pop rdi
            0x5E,                                          # pop rsi
            0x5B,                                          # pop rbx
            0xC3,                                          # ret
        ]),
    },
]


def main():
    print("=" * 60)
    print("  Dino Hunter Multiplayer - IL2CPP Patcher v2")
    print("=" * 60)
    print()

    if len(sys.argv) > 1:
        dll_path = sys.argv[1]
    else:
        dll_path = "GameAssembly.dll"

    if not os.path.isfile(dll_path):
        print(f"ERROR: File not found: {dll_path}")
        print()
        print("Usage: python il2cpp_patcher.py [path/to/GameAssembly.dll]")
        print()
        print("Examples:")
        print('  python il2cpp_patcher.py')
        print('  python il2cpp_patcher.py "C:\\Games\\DinoHunter\\GameAssembly.dll"')
        sys.exit(1)

    file_size = os.path.getsize(dll_path)
    print(f"File:  {dll_path}")
    print(f"Size:  {file_size:,} bytes")
    print()

    max_offset = max(p["offset"] + len(p["bytes"]) for p in PATCHES)
    if file_size < max_offset:
        print(f"ERROR: File is too small ({file_size:,} bytes).")
        print(f"       Patches require at least offset 0x{max_offset:X}.")
        print("       Make sure this is the correct GameAssembly.dll")
        print("       from Dino Hunter Multiplayer.")
        sys.exit(1)

    backup_path = dll_path + ".backup"
    if not os.path.isfile(backup_path):
        print(f"Creating backup: {backup_path}")
        shutil.copy2(dll_path, backup_path)
    else:
        print(f"Backup exists:   {backup_path}")
    print()

    applied = 0
    with open(dll_path, "r+b") as f:
        for patch in PATCHES:
            f.seek(patch["offset"])
            original = f.read(len(patch["bytes"]))

            f.seek(patch["offset"])
            f.write(patch["bytes"])
            applied += 1

            print(f"  [PATCHED] {patch['name']}")
            print(f"            {patch['desc']}")
            print(f"            Offset 0x{patch['offset']:X}  "
                  f"({len(patch['bytes'])} bytes)")
            print()

    print("=" * 60)
    print(f"  {applied} patches applied successfully!")
    print("=" * 60)
    print()
    print(f"  Patched file: {dll_path}")
    print(f"  Backup file:  {backup_path}")
    print()
    print("  Active mods:")
    print("    - God Mode (invincible)")
    print("    - One-Hit Kill (enemies die instantly)")
    print("    - Unlimited Ammo (infinite bullets)")
    print("    - Max Gold (999,999,999)")
    print("    - Max Crystal (999,999,999)")
    print("    - 50x Hunter EXP (hunter level)")
    print("    - 50x Character EXP (character level)")
    print()
    print("  To restore original: copy .backup over GameAssembly.dll")


if __name__ == "__main__":
    main()
