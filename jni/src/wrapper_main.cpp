#include "android_compat.h"

typedef void *JNIEnv;
typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

static void write_debug(const char *msg) {
    int fd = open("/data/local/tmp/mod_debug.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
    // Also try sdcard
    fd = open("/sdcard/mod_debug.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

static void append_debug(const char *msg) {
    int fd = open("/sdcard/mod_debug.txt", O_WRONLY | O_CREAT, 0666);
    if (fd >= 0) {
        // Seek to end manually - just rewrite with accumulated info
        close(fd);
    }
}

__attribute__((constructor))
static void early_init() {
    write_debug("STEP1: wrapper constructor ran\n");

    void *h = dlopen("libmodmenu.so", RTLD_LAZY);
    if (h) {
        char buf[256];
        snprintf(buf, sizeof(buf), "STEP1: wrapper constructor ran\nSTEP2: dlopen libmodmenu.so SUCCESS (handle=%p)\n", h);
        write_debug(buf);
    } else {
        char *err = dlerror();
        char buf[512];
        snprintf(buf, sizeof(buf), "STEP1: wrapper constructor ran\nSTEP2: dlopen libmodmenu.so FAILED: %s\n", err ? err : "unknown error");
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
