# IL2CPP Android Mod (ARMv7)

A native C++ mod framework for Unity IL2CPP games on Android (armeabi-v7a).

## Features

- IL2CPP API resolution (class/method/field lookup)
- Thumb/ARM inline function hooking
- Hook by offset or by class/method name
- Mod menu system with toggles and sliders
- Example hooks: god mode, one-hit kill, unlimited ammo, speed hack

## Project Structure

```
jni/
  include/
    il2cpp.h      - IL2CPP API wrapper
    hook.h         - Hook engine interface
    menu.h         - Mod menu interface
    log.h          - Android logging macros
  src/
    main.cpp       - Entry point (JNI_OnLoad + constructor)
    il2cpp.cpp     - IL2CPP API resolution via dlsym
    hooks.cpp      - Game-specific hook definitions (edit this)
    menu.cpp       - Mod menu implementation
    thumbhook.cpp  - Thumb/ARM inline hook engine
  Android.mk       - NDK build config
  Application.mk   - NDK app config (ABI, STL)
CMakeLists.txt     - Alternative CMake build
build.sh           - Build script
```

## Prerequisites

- [Android NDK](https://developer.android.com/ndk/downloads) (r21+)
- [Il2CppDumper](https://github.com/Perfare/Il2CppDumper) (to get method offsets)
- [apktool](https://apktool.org/) (to decompile/rebuild APK)

## How to Use

### Step 1: Dump the Game

1. Extract `libil2cpp.so` and `global-metadata.dat` from the game APK
2. Run Il2CppDumper to generate `dump.cs`
3. Find the classes and methods you want to hook in `dump.cs`

### Step 2: Configure Hooks

Edit `jni/src/hooks.cpp`:

**Option A — Hook by offset** (from the `// RVA:` comments in dump.cs):
```cpp
hook::hook_addr(base + 0x123456, (void *)hook_TakeDamage, (void **)&orig_TakeDamage);
```

**Option B — Hook by name** (more portable across updates):
```cpp
auto *klass = il2cpp::find_class("", "Player");
auto *method = il2cpp::class_get_method_from_name(klass, "TakeDamage", 1);
hook::hook_method(il2cpp::get_method_pointer(method), hook_TakeDamage, &orig_TakeDamage);
```

### Step 3: Build

```bash
export ANDROID_NDK_HOME=/path/to/ndk
./build.sh
```

Output: `libs/armeabi-v7a/libmodmenu.so`

### Step 4: Install into APK

1. Decompile the APK:
   ```bash
   apktool d game.apk -o game
   ```

2. Copy the mod library:
   ```bash
   cp libs/armeabi-v7a/libmodmenu.so game/lib/armeabi-v7a/
   ```

3. Add a load call in the game's main Activity smali. Find the `onCreate` method and add:
   ```smali
   const-string v0, "modmenu"
   invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V
   ```
   Alternatively, the mod uses `__attribute__((constructor))` so it will auto-initialize
   if loaded by the linker (add it to the APK's `lib/` folder and reference it).

4. Rebuild and sign:
   ```bash
   apktool b game -o modded.apk
   # Sign with your keystore
   ```

## Writing Custom Hooks

Each hook follows this pattern:

```cpp
// 1. Define the original function type
typedef float (*GetHealth_t)(void *self, void *method);
static GetHealth_t orig_GetHealth = nullptr;

// 2. Write the hook function (same signature)
float hook_GetHealth(void *self, void *method) {
    if (menu::get_toggle(MY_TOGGLE)) {
        return 9999.0f;  // modified behavior
    }
    return orig_GetHealth(self, method);  // original behavior
}

// 3. Install it in install_hooks()
hook::hook_addr(base + 0xOFFSET, (void *)hook_GetHealth, (void **)&orig_GetHealth);
```

### IL2CPP Method Signatures

In IL2CPP, every method gets an extra `void *method` parameter at the end (the MethodInfo pointer). Instance methods also have `void *self` as the first parameter.

| C# Signature | C++ Hook Signature |
|---|---|
| `void Foo()` | `void hook_Foo(void *self, void *method)` |
| `int Bar(float x)` | `int hook_Bar(void *self, float x, void *method)` |
| `static bool Baz(int a, int b)` | `bool hook_Baz(int a, int b, void *method)` |

## Debugging

View logs with:
```bash
adb logcat -s IL2CPP_MOD
```
