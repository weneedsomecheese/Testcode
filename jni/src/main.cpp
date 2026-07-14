#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

static void *mod_thread(void *) {
    LOGI("=== Mod thread alive, doing nothing ===");
    return (void *)0;
}

__attribute__((constructor))
static void lib_entry() {
    LOGI("=== IL2CPP Mod loaded ===");

    pthread_t tid;
    pthread_create(&tid, 0, mod_thread, 0);
    pthread_detach(tid);
}
