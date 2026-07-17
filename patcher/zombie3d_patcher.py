#!/usr/bin/env python3
"""
Zombie3D Game - IL2CPP GameAssembly.dll Patcher v3

Patches native x86-64 code in GameAssembly.dll for:
  1. Max Currency      - All SafeInteger values read as 999,999,999
                         (cash, crystal, ammo, medpacks - all maxed)
  2. Free Spending     - Spending cash/crystals doesn't decrease them
  3. Sane Weapon Dmg   - Weapon damage capped at config max (not infinity)
  4. Day 231+          - Day counter starts at 231 and keeps going up
  5. Tamper Detection   - Disable client-side cheat detection

Usage:
  python zombie3d_patcher.py                        (looks for GameAssembly.dll in current dir)
  python zombie3d_patcher.py C:\Games\Zombie\GameAssembly.dll

IMPORTANT: Restore from .backup before re-patching if you previously
           applied an older version of this patcher.

To restore the original, rename GameAssembly.dll.backup back to GameAssembly.dll.
"""

import shutil
import sys
import os

PATCHES = [
    #
    # ── Currency: SafeInteger always returns 999M ──
    # (needed so buy/upgrade internal checks pass)
    #
    {
        "name": "Max SafeInteger (Get)",
        "desc": "SafeInteger.Get -> always return 999999999",
        "offset": 0x3B96D0,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Max SafeInteger (implicit->int)",
        "desc": "SafeInteger.op_Implicit(SafeInteger)->int -> always return 999999999",
        "offset": 0x3B9BF0,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Max Cash (display)",
        "desc": "GameState.GetCash -> always return 999999999",
        "offset": 0x40AF20,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Max Crystal (display)",
        "desc": "GameState.GetCrystal -> always return 999999999",
        "offset": 0x40AF40,
        "bytes": bytes([
            0xB8, 0xFF, 0xC9, 0x9A, 0x3B,  # mov eax, 999999999
            0xC3,                            # ret
        ]),
    },
    {
        "name": "Free Spending (Cash)",
        "desc": "GameState.LoseCash -> do nothing",
        "offset": 0x40E640,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    {
        "name": "Free Spending (Crystal)",
        "desc": "GameState.LoseCrystal -> do nothing",
        "offset": 0x40E6C0,
        "bytes": bytes([
            0xC3,  # ret
        ]),
    },
    #
    # ── Fix weapon damage: return config max instead of infinity ──
    # Without this, SafeInteger making level=999M causes damage formula
    # to produce infinity. This returns WConf.damageFinal (max-level dmg).
    #
    {
        "name": "Weapon Damage Fix",
        "desc": "Weapon.get_AttackDamage -> return WConf.damageFinal (max-level damage, not inf)",
        "offset": 0x37C400,
        "bytes": bytes([
            # mov rax, [rcx+0xE8]            ; this.WConf (WeaponConfig)
            0x48, 0x8B, 0x81, 0xE8, 0x00, 0x00, 0x00,
            # mov rcx, [rax+0x68]            ; WConf.damageFinal (SafeFloat)
            0x48, 0x8B, 0x48, 0x68,
            # jmp SafeFloat.Get (RVA 0x3B9D50, relative from RVA 0x37D210)
            0xE9, 0x40, 0xCB, 0x03, 0x00,
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
            # mov eax, [rcx+0x118]
            0x8B, 0x81, 0x18, 0x01, 0x00, 0x00,
            # cmp eax, 231
            0x3D, 0xE7, 0x00, 0x00, 0x00,
            # jge +5
            0x7D, 0x05,
            # mov eax, 231
            0xB8, 0xE7, 0x00, 0x00, 0x00,
            # ret
            0xC3,
        ]),
    },
    {
        "name": "Day 231+ (setter)",
        "desc": "GameState.set_LevelNum -> enforce minimum 231",
        "offset": 0x410470,
        "bytes": bytes([
            # cmp edx, 231
            0x81, 0xFA, 0xE7, 0x00, 0x00, 0x00,
            # jge +6
            0x7D, 0x06,
            # add edx, 230
            0x81, 0xC2, 0xE6, 0x00, 0x00, 0x00,
            # mov [rcx+0x118], edx
            0x89, 0x91, 0x18, 0x01, 0x00, 0x00,
            # ret
            0xC3,
        ]),
    },
    {
        "name": "Day 231+ (DayUp)",
        "desc": "GameState.DayUp -> increment LevelNum, enforce minimum 231",
        "offset": 0x40A3B0,
        "bytes": bytes([
            # mov eax, [rcx+0x118]
            0x8B, 0x81, 0x18, 0x01, 0x00, 0x00,
            # inc eax
            0xFF, 0xC0,
            # cmp eax, 231
            0x3D, 0xE7, 0x00, 0x00, 0x00,
            # jge +5
            0x7D, 0x05,
            # mov eax, 231
            0xB8, 0xE7, 0x00, 0x00, 0x00,
            # mov [rcx+0x118], eax
            0x89, 0x81, 0x18, 0x01, 0x00, 0x00,
            # ret
            0xC3,
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
    print("  Zombie3D Game - IL2CPP Patcher v3")
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
    print("    - Max Currency (999M cash, crystal, ammo, medpacks)")
    print("    - Free Spending (buying costs nothing)")
    print("    - Buying/upgrading always works")
    print("    - Weapon damage = max-level config value (not infinity)")
    print("    - Day 231+ (keeps going up)")
    print("    - Client-side tamper detection disabled")
    print()
    print("  NOTES:")
    print("    - Weapon/character levels display as very high (cosmetic)")
    print("    - Character HP/damage will be high (basically god mode)")
    print("    - Avoid PvP/boss raid to prevent server-side detection")
    print("    - Music toggle in settings may not work")
    print()
    print("  To restore original: copy .backup over GameAssembly.dll")


if __name__ == "__main__":
    main()
