#include "android_compat.h"

typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

typedef struct {
    const char *dli_fname;
    void       *dli_fbase;
    const char *dli_sname;
    void       *dli_saddr;
} Dl_info;

extern "C" int dladdr(const void *addr, Dl_info *info);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

static void build_path(char *out, const char *dir_from, const char *filename) {
    const char *last_slash = nullptr;
    for (const char *p = dir_from; *p; p++) {
        if (*p == '/') last_slash = p;
    }
    int i = 0;
    if (last_slash) {
        for (const char *p = dir_from; p <= last_slash && i < 480; p++)
            out[i++] = *p;
    }
    for (const char *p = filename; *p && i < 510; p++)
        out[i++] = *p;
    out[i] = '\0';
}

__attribute__((constructor))
static void early_init() {
    void *h = nullptr;

    Dl_info info;
    if (dladdr((void *)early_init, &info) && info.dli_fname) {
        char path[512];
        build_path(path, info.dli_fname, "libmodmenu.so");
        h = dlopen(path, RTLD_LAZY);
    }

    if (!h)
        h = dlopen("libmodmenu.so", RTLD_LAZY);

    if (!h) {
        // CRASH = dlopen failed (libmodmenu.so can't be loaded)
        // NO CRASH = dlopen succeeded (mod loaded, issue is inside mod)
        volatile int *p = (volatile int *)0;
        *p = 42;
    }
}

extern "C" __attribute__((visibility("default")))
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    if (!orig_JNI_OnLoad) {
        Dl_info info;
        if (dladdr((void *)early_init, &info) && info.dli_fname) {
            char path[512];
            build_path(path, info.dli_fname, "libmain_orig.so");
            void *h = dlopen(path, RTLD_LAZY);
            if (h) orig_JNI_OnLoad = reinterpret_cast<JNI_OnLoad_t>(dlsym(h, "JNI_OnLoad"));
        }
        if (!orig_JNI_OnLoad) {
            void *h = dlopen("libmain_orig.so", RTLD_LAZY);
            if (h) orig_JNI_OnLoad = reinterpret_cast<JNI_OnLoad_t>(dlsym(h, "JNI_OnLoad"));
        }
    }
    if (orig_JNI_OnLoad)
        return orig_JNI_OnLoad(vm, reserved);
    return 0x00010006;
}
