#include "android_compat.h"

typedef void *JNIEnv;
typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

// Forward declarations from other translation units
extern void install_hooks();

namespace il2cpp {
    bool init(const char *lib_name);
    void *domain_get();
    void *thread_attach(void *domain);
    uintptr_t get_base_address();
}

namespace hook {
    bool init();
}

namespace menu {
    void set_toggle(int index, bool value);
    void set_slider(int index, int value);
}

// Logging - direct dlsym approach
typedef int (*log_fn_t)(int prio, const char *tag, const char *fmt, ...);
static log_fn_t g_log_fn = nullptr;

static void init_log() {
    void *h = dlopen("liblog.so", RTLD_LAZY);
    if (h) g_log_fn = reinterpret_cast<log_fn_t>(dlsym(h, "__android_log_print"));
}

#define MOD_LOG(prio, ...) do { if (g_log_fn) g_log_fn(prio, "IL2CPP_MOD", __VA_ARGS__); } while(0)
#define MOD_LOGI(...) MOD_LOG(4, __VA_ARGS__)
#define MOD_LOGE(...) MOD_LOG(6, __VA_ARGS__)

static void *mod_thread(void *) {
    init_log();
    MOD_LOGI("=== Mod thread started ===");

    int attempts = 0;
    while (!il2cpp::init("libil2cpp.so")) {
        usleep(500000);
        attempts++;
        if (attempts % 10 == 0) {
            MOD_LOGI("Still waiting for libil2cpp.so (attempt %d)...", attempts);
        }
        if (attempts > 120) {
            MOD_LOGE("Gave up waiting for libil2cpp.so after 60s");
            return (void *)0;
        }
    }

    MOD_LOGI("libil2cpp.so found! Base: 0x%lx", (unsigned long)il2cpp::get_base_address());

    auto *domain = il2cpp::domain_get();
    if (domain) {
        il2cpp::thread_attach(domain);
        MOD_LOGI("Thread attached to IL2CPP domain");
    } else {
        MOD_LOGE("Failed to get IL2CPP domain");
    }

    hook::init();

    menu::set_toggle(0, true);
    menu::set_toggle(1, true);
    menu::set_toggle(2, true);
    menu::set_toggle(3, true);
    menu::set_toggle(4, true);
    menu::set_toggle(5, true);
    menu::set_toggle(6, true);
    menu::set_slider(0, 10);
    menu::set_slider(1, 10);

    install_hooks();

    MOD_LOGI("=== All mods activated! ===");
    return (void *)0;
}

__attribute__((constructor))
static void lib_entry() {
    pthread_t tid;
    pthread_create(&tid, 0, mod_thread, 0);
    pthread_detach(tid);
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
