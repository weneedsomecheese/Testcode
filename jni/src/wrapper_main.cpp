#include "android_compat.h"

typedef void *JNIEnv;
typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

static char g_debug_path[512] = {0};

static void init_debug_path() {
    char cmdline[256] = {0};
    int fd = open("/proc/self/cmdline", O_RDONLY, 0);
    if (fd >= 0) {
        read(fd, cmdline, sizeof(cmdline) - 1);
        close(fd);
    }

    // Build path: /sdcard/Android/data/<package>/files/mod_debug.txt
    if (cmdline[0]) {
        strcpy(g_debug_path, "/sdcard/Android/data/");
        strcat(g_debug_path, cmdline);
        strcat(g_debug_path, "/files");
        mkdir(g_debug_path, 0777);
        strcat(g_debug_path, "/mod_debug.txt");
    }
}

static void write_debug(const char *msg) {
    // Try app-specific external storage (readable without root)
    if (g_debug_path[0]) {
        int fd = open(g_debug_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd >= 0) {
            write(fd, msg, strlen(msg));
            close(fd);
        }
    }
    // Also try sdcard root and /data/local/tmp as fallbacks
    int fd = open("/sdcard/mod_debug.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
    fd = open("/data/local/tmp/mod_debug.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

__attribute__((constructor))
static void early_init() {
    init_debug_path();
    write_debug("STEP1: wrapper constructor ran\n");

    void *h = dlopen("libmodmenu.so", RTLD_LAZY);
    if (h) {
        char buf[512];
        snprintf(buf, sizeof(buf),
            "STEP1: wrapper constructor ran\n"
            "STEP2: dlopen libmodmenu.so SUCCESS (handle=%p)\n"
            "DEBUG_PATH: %s\n",
            h, g_debug_path);
        write_debug(buf);
    } else {
        char *err = dlerror();
        char buf[768];
        snprintf(buf, sizeof(buf),
            "STEP1: wrapper constructor ran\n"
            "STEP2: dlopen libmodmenu.so FAILED: %s\n"
            "DEBUG_PATH: %s\n",
            err ? err : "unknown error", g_debug_path);
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
