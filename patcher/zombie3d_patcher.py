#!/usr/bin/env python3
"""
Zombie3D Game - IL2CPP GameAssembly.dll Patcher

Patches native x86-64 code in GameAssembly.dll for:
  1. Max Cash (Gold)  - Always shows 999,999,999 gold
  2. Max Crystal      - Always shows 999,999,999 crystals
  3. Free Spending    - Spending cash/crystals doesn't decrease them
  4. Day 231+         - Day counter starts at 231 and keeps going up
  5. Damage x5        - Player weapon damage multiplied by 5

Usage:
  python zombie3d_patcher.py                        (looks for GameAssembly.dll in current dir)
  python zombie3d_patcher.py C:\Games\Zombie\GameAssembly.dll

To restore the original, rename GameAssembly.dll.backup back to GameAssembly.dll.
"""

import shutil
import sys
import os

PATCHES = [
    {
        "name": "Max Cash (999,999,999)",
        "desc": "GameState.GetCash -> always return 999999999",
        "offset": 0x40AF20,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999 (0x3B9AC9FF)
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Max Crystal (999,999,999)",
        "desc": "GameState.GetCrystal -> always return 999999999",
        "offset": 0x40AF40,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999 (0x3B9AC9FF)
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Free Spending (Cash)",
        "desc": "GameState.LoseCash -> do nothing (spending doesn't subtract)",
        "offset": 0x40E640,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    {
        "name": "Free Spending (Crystal)",
        "desc": "GameState.LoseCrystal -> do nothing (spending doesn't subtract)",
        "offset": 0x40E6C0,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    {
        "name": "Day 231+",
        "desc": "GameState.get_LevelNum -> return actual day + 230 (so day 1 = 231, keeps going up)",
        "offset": 0x33D870,
        "bytes": bytes([
            # mov eax, dword ptr [rcx+0x118]   ; load actual LevelNum
            0x8B, 0x81, 0x18, 0x01, 0x00, 0x00,
            # add eax, 230                      ; add 230 offset
            0x05, 0xE6, 0x00, 0x00, 0x00,
            # ret
            0xC3,
        ]),
    },
    {
        "name": "Damage x5",
        "desc": "Player.get_Damage -> return base damage * 5.0",
        "offset": 0x34FED0,
        "bytes": bytes([
            # movss xmm0, dword ptr [rcx+0x64]  ; load this->damage
            0xF3, 0x0F, 0x10, 0x41, 0x64,
            # mov eax, 0x40A00000                ; 5.0f
            0xB8, 0x00, 0x00, 0xA0, 0x40,
            # movd xmm1, eax
            0x66, 0x0F, 0x6E, 0xC8,
            # mulss xmm0, xmm1                   ; damage * 5
            0xF3, 0x0F, 0x59, 0xC1,
            # ret
            0xC3,
        ]),
    },
]


def main():
    print("=" * 60)
    print("  Zombie3D Game - IL2CPP Patcher")
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
    print("    - Max Cash: 999,999,999 gold")
    print("    - Max Crystal: 999,999,999 crystals")
    print("    - Free Spending: buying things costs nothing")
    print("    - Day 231+ (keeps going up: 231, 232, 233...)")
    print("    - Damage x5 (weapon damage multiplied by 5)")
    print()
    print("  To restore original: copy .backup over GameAssembly.dll")


if __name__ == "__main__":
    main()
