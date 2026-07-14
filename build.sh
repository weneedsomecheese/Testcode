#!/bin/bash
#
# Build the mod .so library
#
# Works with:
#   - Android NDK on PC (set ANDROID_NDK_HOME)
#   - Termux on Android (auto-detected)
#
# Usage:
#   ./build.sh          # Build the mod
#   ./build.sh clean    # Clean build artifacts
#

set -e

# --- Detect environment ---
if [ -d "$PREFIX" ] && [ -f "$PREFIX/bin/termux-info" ]; then
    echo "=== Termux detected ==="
    BUILD_MODE="termux"
else
    BUILD_MODE="ndk"
fi

# --- Clean ---
if [ "$1" = "clean" ]; then
    echo "Cleaning..."
    rm -rf libs obj build
    echo "Done."
    exit 0
fi

if [ "$BUILD_MODE" = "termux" ]; then
    # =============================================
    # TERMUX BUILD
    # =============================================
    for pkg in clang make; do
        if ! command -v $pkg &>/dev/null; then
            echo "Installing $pkg..."
            pkg install -y $pkg
        fi
    done

    SOURCES="jni/src/main.cpp jni/src/il2cpp.cpp jni/src/hooks.cpp jni/src/menu.cpp jni/src/thumbhook.cpp"

    ARCH=$(uname -m)
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
        OUTDIR="libs/arm64-v8a"
        ARCHFLAG=""
        echo "Detected ARM64 phone — building arm64-v8a .so"
        echo "(Put it in lib/arm64-v8a/ in the APK)"
    else
        OUTDIR="libs/armeabi-v7a"
        ARCHFLAG="-mthumb"
        echo "Detected ARM32 phone — building armeabi-v7a .so"
    fi

    mkdir -p "$OUTDIR"

    echo "Building with: clang++"
    clang++ \
        -shared -o "$OUTDIR/libmodmenu.so" \
        $SOURCES \
        -Ijni/include \
        -std=c++17 \
        -O2 \
        -fvisibility=hidden \
        -fno-rtti \
        -fno-exceptions \
        -fPIC \
        -DANDROID \
        -static-libstdc++ \
        $ARCHFLAG \
        -s

    echo ""
    echo "Build complete!"
    echo "Output: $OUTDIR/libmodmenu.so"
    echo "Size: $(du -h "$OUTDIR/libmodmenu.so" | cut -f1)"
    echo ""
    echo "Now use MT Manager to:"
    echo "  1. Open the game APK"
    if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
        echo "  2. Check if lib/arm64-v8a/ exists in the APK"
        echo "     - If YES: add libmodmenu.so to lib/arm64-v8a/"
        echo "     - If NO (only armeabi-v7a): you need a PC with NDK to cross-compile"
    else
        echo "  2. Add libmodmenu.so to lib/armeabi-v7a/"
    fi
    echo "  3. Inject loadLibrary in the dex"
    echo "  4. Save, sign, install"

else
    # =============================================
    # NDK BUILD (PC)
    # =============================================
    NDK="${ANDROID_NDK_HOME:-${NDK_HOME:-$HOME/android-ndk}}"

    if [ ! -d "$NDK" ]; then
        echo "ERROR: Android NDK not found."
        echo "Set ANDROID_NDK_HOME to your NDK installation path."
        echo "Example: export ANDROID_NDK_HOME=\$HOME/Android/Sdk/ndk/25.2.9519653"
        exit 1
    fi

    NDBUILD="$NDK/ndk-build"

    echo "Building with NDK: $NDK"
    "$NDBUILD" NDK_PROJECT_PATH=. -j$(nproc)

    echo ""
    echo "Build complete!"
    echo "Output: libs/armeabi-v7a/libmodmenu.so"
fi
