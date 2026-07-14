#!/bin/bash
#
# Build the mod .so library using Android NDK
#
# Requirements:
#   - Android NDK installed (r21+ recommended)
#   - ANDROID_NDK_HOME or NDK_HOME environment variable set
#
# Usage:
#   ./build.sh          # Build the mod
#   ./build.sh clean    # Clean build artifacts
#

set -e

NDK="${ANDROID_NDK_HOME:-${NDK_HOME:-$HOME/android-ndk}}"

if [ ! -d "$NDK" ]; then
    echo "ERROR: Android NDK not found."
    echo "Set ANDROID_NDK_HOME to your NDK installation path."
    echo "Example: export ANDROID_NDK_HOME=\$HOME/Android/Sdk/ndk/25.2.9519653"
    exit 1
fi

NDBUILD="$NDK/ndk-build"

if [ "$1" = "clean" ]; then
    echo "Cleaning..."
    "$NDBUILD" NDK_PROJECT_PATH=. clean
    rm -rf libs obj
    echo "Done."
    exit 0
fi

echo "Building with NDK: $NDK"
"$NDBUILD" NDK_PROJECT_PATH=. -j$(nproc)

echo ""
echo "Build complete!"
echo "Output: libs/armeabi-v7a/libmodmenu.so"
echo ""
echo "To use:"
echo "  1. Decompile the APK:  apktool d game.apk"
echo "  2. Copy libs/armeabi-v7a/libmodmenu.so to game/lib/armeabi-v7a/"
echo "  3. Add System.loadLibrary(\"modmenu\") to the smali (see README)"
echo "  4. Rebuild & sign:    apktool b game -o modded.apk"
