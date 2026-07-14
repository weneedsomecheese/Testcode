#include <jni.h>
#include <pthread.h>
#include <unistd.h>

#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

static bool initialized = false;

static void *mod_thread(void *) {
    LOGI("Mod thread started, waiting for il2cpp...");

    // Wait for libil2cpp.so to be loaded by the game
    while (!il2cpp::init("libil2cpp.so")) {
        usleep(500000); // 500ms
    }

    // Attach to the il2cpp thread
    auto *domain = il2cpp::domain_get();
    if (domain) {
        il2cpp::thread_attach(domain);
    }

    // Initialize the hook engine
    hook::init();

    // Install all hooks
    install_hooks();

    LOGI("Mod fully initialized!");
    initialized = true;
    return nullptr;
}

__attribute__((constructor))
static void lib_entry() {
    LOGI("=== IL2CPP Mod loaded ===");

    pthread_t tid;
    pthread_create(&tid, nullptr, mod_thread, nullptr);
    pthread_detach(tid);
}

// JNI entry point — called when the library is loaded via System.loadLibrary
extern "C" jint JNI_OnLoad(JavaVM *vm, void *) {
    LOGI("JNI_OnLoad called");

    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        LOGE("Failed to get JNI env");
        return JNI_ERR;
    }

    return JNI_VERSION_1_6;
}
