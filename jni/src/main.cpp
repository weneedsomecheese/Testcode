#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

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
    LOGI("domain_get returned: %p", (void *)domain);

    if (domain) {
        il2cpp::thread_attach(domain);
        LOGI("Thread attached to IL2CPP domain");
    }

    LOGI("=== Test done, domain_get + thread_attach only ===");
    return (void *)0;
}

__attribute__((constructor))
static void lib_entry() {
    LOGI("=== IL2CPP Mod loaded ===");

    pthread_t tid;
    pthread_create(&tid, 0, mod_thread, 0);
    pthread_detach(tid);
}
