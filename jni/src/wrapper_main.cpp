#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

typedef void *JavaVM;
typedef int jint;
typedef jint (*JNI_OnLoad_t)(JavaVM *vm, void *reserved);

static JNI_OnLoad_t orig_JNI_OnLoad = nullptr;

static void *mod_thread(void *) {
    LOGI("Mod thread started, waiting for il2cpp...");

    int attempts = 0;
    while (!il2cpp::init("libil2cpp.so")) {
        usleep(500000);
        attempts++;
        if (attempts > 120) {
            LOGE("Gave up waiting for libil2cpp.so after 60s");
            return (void *)0;
        }
    }

    LOGI("libil2cpp.so found! Base: 0x%lx", (unsigned long)il2cpp::get_base_address());

    auto *domain = il2cpp::domain_get();
    if (domain) {
        il2cpp::thread_attach(domain);
        LOGI("Thread attached to IL2CPP domain");
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

    LOGI("=== Mod fully initialized! All mods ON ===");
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
