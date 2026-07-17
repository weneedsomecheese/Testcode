#!/usr/bin/env python3
"""
Zombie3D Game - IL2CPP GameAssembly.dll Patcher v5

Patches native x86-64 code in GameAssembly.dll for:
  1. Reverse Spending  - Spending cash/crystals GIVES you money instead
  2. Day 231+          - Day counter starts at 231 and keeps going up
  3. Tamper Detection  - Disable client-side cheat detection

How it works:
  - LoseCash jumps to AddCash -> spending gives you cash
  - LoseCrystal jumps to AddCrystal -> spending gives you crystals
  - Your balance changes naturally (no forced 999M)

Usage:
  python zombie3d_patcher.py                        (looks for GameAssembly.dll in current dir)
  python zombie3d_patcher.py C:\Games\Zombie\GameAssembly.dll

IMPORTANT: Restore from .backup before re-patching if you previously
           applied an older version of this patcher.
"""

import shutil
import sys
import os

PATCHES = [
    #
    # ── Currency: LoseCash/LoseCrystal redirect to AddCash/AddCrystal ──
    #
    {
        "name": "LoseCash -> AddCash (reverse)",
        "desc": "GameState.LoseCash -> jmp to AddCash (spending gives you cash instead)",
        "offset": 0x40E640,
        "bytes": bytes([
            # jmp AddCash (RVA 0x40A6D0, relative from RVA 0x40F445)
            0xE9, 0x8B, 0xB2, 0xFF, 0xFF,
        ]),
    },
    {
        "name": "LoseCrystal -> AddCrystal (reverse)",
        "desc": "GameState.LoseCrystal -> jmp to AddCrystal (spending gives you crystals instead)",
        "offset": 0x40E6C0,
        "bytes": bytes([
            # jmp AddCrystal (RVA 0x40A790, relative from RVA 0x40F4C5)
            0xE9, 0xCB, 0xB2, 0xFF, 0xFF,
        ]),
    },
    #
    # ── Day 231+ ──
    #
    {
        "name": "Day 231+ (getter)",
        "desc": "GameState.get_LevelNum -> return max(actual, 231)",
        "offset": 0x33D870,
        "bytes": bytes([
            0x8B, 0x81, 0x18, 0x01, 0x00, 0x00,  # mov eax, [rcx+0x118]
            0x3D, 0xE7, 0x00, 0x00, 0x00,          # cmp eax, 231
            0x7D, 0x05,                              # jge +5
            0xB8, 0xE7, 0x00, 0x00, 0x00,          # mov eax, 231
            0xC3,                                    # ret
        ]),
    },
    {
        "name": "Day 231+ (setter)",
        "desc": "GameState.set_LevelNum -> enforce minimum 231",
        "offset": 0x410470,
        "bytes": bytes([
            0x81, 0xFA, 0xE7, 0x00, 0x00, 0x00,  # cmp edx, 231
            0x7D, 0x06,                            # jge +6
            0x81, 0xC2, 0xE6, 0x00, 0x00, 0x00,  # add edx, 230
            0x89, 0x91, 0x18, 0x01, 0x00, 0x00,  # mov [rcx+0x118], edx
            0xC3,                                  # ret
        ]),
    },
    {
        "name": "Day 231+ (DayUp)",
        "desc": "GameState.DayUp -> increment LevelNum, enforce minimum 231",
        "offset": 0x40A3B0,
        "bytes": bytes([
            0x8B, 0x81, 0x18, 0x01, 0x00, 0x00,  # mov eax, [rcx+0x118]
            0xFF, 0xC0,                            # inc eax
            0x3D, 0xE7, 0x00, 0x00, 0x00,          # cmp eax, 231
            0x7D, 0x05,                              # jge +5
            0xB8, 0xE7, 0x00, 0x00, 0x00,          # mov eax, 231
            0x89, 0x81, 0x18, 0x01, 0x00, 0x00,  # mov [rcx+0x118], eax
            0xC3,                                  # ret
        ]),
    },
    #
    # ── Anti-tamper bypass ──
    #
    {
        "name": "Disable TamperDetector",
        "desc": "TamperDetector.IsTamperDetected -> return false",
        "offset": 0x3742F0,
        "bytes": bytes([
            0x31, 0xC0,  # xor eax, eax
            0xC3,        # ret
        ]),
    },
    {
        "name": "Disable TamperWatcher",
        "desc": "TamperWatcher.Update -> do nothing",
        "offset": 0x374D30,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    {
        "name": "Disable VMDetector",
        "desc": "VMDetector.IsVM -> return false",
        "offset": 0x378D40,
        "bytes": bytes([
            0x31, 0xC0,  # xor eax, eax
            0xC3,        # ret
        ]),
    },
]


def main():
    print("=" * 60)
    print("  Zombie3D Game - IL2CPP Patcher v5")
    print("=" * 60)
    print()

    if len(sys.argv) > 1:
        dll_path = sys.argv[1]
    else:
        dll_path = "GameAssembly.dll"

    if not os.path.isfile(dll_path):
        print(f"ERROR: File not found: {dll_path}")
        print()
        print("Usage: python zombie3d_patcher.py [path/to/GameAssembly.dll]")
        print()
        print("Examples:")
        print('  python zombie3d_patcher.py')
        print('  python zombie3d_patcher.py "C:\\Games\\Zombie\\GameAssembly.dll"')
        sys.exit(1)

    file_size = os.path.getsize(dll_path)
    print(f"File:  {dll_path}")
    print(f"Size:  {file_size:,} bytes")
    print()

    max_offset = max(p["offset"] + len(p["bytes"]) for p in PATCHES)
    if file_size < max_offset:
        print(f"ERROR: File is too small ({file_size:,} bytes).")
        print(f"       Patches require at least offset 0x{max_offset:X}.")
        print("       Make sure this is the correct GameAssembly.dll.")
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
    print("    - Spending GIVES you money (LoseCash -> AddCash)")
    print("    - Day 231+ (keeps going up)")
    print("    - Client-side tamper detection disabled")
    print()
    print("  HOW IT WORKS:")
    print("    Every purchase adds the cost to your balance instead of")
    print("    subtracting it. Your currency grows naturally as you buy.")
    print()
    print("  Weapon stats, character stats, and spawns are NOT modified.")
    print()
    print("  To restore original: copy .backup over GameAssembly.dll")


if __name__ == "__main__":
    main()
