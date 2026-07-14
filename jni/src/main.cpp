#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

static void *mod_thread(void *) {
    LOGI("Mod thread started, waiting for il2cpp...");

    while (!il2cpp::init("libil2cpp.so")) {
        usleep(500000);
    }

    auto *domain = il2cpp::domain_get();
    if (domain) {
        il2cpp::thread_attach(domain);
    }

    hook::init();

    // Enable mods by default
    menu::set_toggle(0, true);  // gold multiply
    menu::set_toggle(1, true);  // crystal multiply
    menu::set_toggle(2, true);  // god mode
    menu::set_toggle(3, true);  // one-hit kill
    menu::set_toggle(4, true);  // unlimited ammo
    menu::set_toggle(5, true);  // exp multiply
    menu::set_toggle(6, true);  // damage multiply
    menu::set_slider(0, 10);    // 10x gold/crystal/exp
    menu::set_slider(1, 10);    // 10x damage

    install_hooks();

    LOGI("=== Mod fully initialized! All mods ON ===");
    return (void *)0;
}

__attribute__((constructor))
static void lib_entry() {
    LOGI("=== IL2CPP Mod loaded ===");

    pthread_t tid;
    pthread_create(&tid, 0, mod_thread, 0);
    pthread_detach(tid);
}
