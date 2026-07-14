#include "android_compat.h"

typedef void *JNIEnv;
typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

#define PKG "com.DecompAndRecomp.DinoHunterMultiplayer"

static void try_write(const char *path, const char *msg) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

static void write_debug(const char *msg) {
    // Internal app data - most reliable
    try_write("/data/data/" PKG "/mod_debug.txt", msg);

    // External app-specific - create dir chain first
    mkdir("/sdcard/Android/data/" PKG, 0777);
    mkdir("/sdcard/Android/data/" PKG "/files", 0777);
    try_write("/sdcard/Android/data/" PKG "/files/mod_debug.txt", msg);

    mkdir("/storage/emulated/0/Android/data/" PKG, 0777);
    mkdir("/storage/emulated/0/Android/data/" PKG "/files", 0777);
    try_write("/storage/emulated/0/Android/data/" PKG "/files/mod_debug.txt", msg);

    // Fallbacks
    try_write("/sdcard/mod_debug.txt", msg);
    try_write("/storage/emulated/0/mod_debug.txt", msg);
    try_write("/data/local/tmp/mod_debug.txt", msg);
}

__attribute__((constructor))
static void early_init() {
    write_debug("STEP1: wrapper constructor ran\n");

    void *h = dlopen("libmodmenu.so", RTLD_LAZY);
    if (h) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "STEP1: wrapper constructor ran\n"
            "STEP2: dlopen libmodmenu.so SUCCESS (handle=%p)\n", h);
        write_debug(buf);
    } else {
        char *err = dlerror();
        char buf[512];
        snprintf(buf, sizeof(buf),
            "STEP1: wrapper constructor ran\n"
            "STEP2: dlopen libmodmenu.so FAILED: %s\n",
            err ? err : "unknown error");
        write_debug(buf);
    }
}

extern "C" __attribute__((visibility("default")))
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (!orig_JNI_OnLoad) {
        void *h = dlopen("libmain_orig.so", RTLD_LAZY);
        if (h) orig_JNI_OnLoad = reinterpret_cast<JNI_OnLoad_t>(dlsym(h, "JNI_OnLoad"));
    }
    if (orig_JNI_OnLoad)
        return orig_JNI_OnLoad(vm, reserved);
    return 0x00010006;
}
