#include "il2cpp.h"
#include "hook.h"
#include "menu.h"
#include "log.h"

extern void install_hooks();

static void debug_write(const char *msg) {
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

static void *mod_thread(void *) {
    LOGI("Mod thread started, waiting for il2cpp...");
    debug_write("STEP3: mod_thread started, waiting for libil2cpp.so\n");

    int attempts = 0;
    while (!il2cpp::init("libil2cpp.so")) {
        usleep(500000);
        attempts++;
        if (attempts > 120) {
            debug_write("STEP3: mod_thread started\nSTEP4: FAILED - gave up waiting for libil2cpp.so after 60s\n");
            return (void *)0;
        }
    }

    uintptr_t base = il2cpp::get_base_address();
    char buf[512];
    snprintf(buf, sizeof(buf),
        "STEP3: mod_thread started\n"
        "STEP4: libil2cpp.so found after %d attempts, base=0x%lx\n",
        attempts, (unsigned long)base);
    debug_write(buf);

    auto *domain = il2cpp::domain_get();
    if (domain) {
        il2cpp::thread_attach(domain);
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

    snprintf(buf, sizeof(buf),
        "STEP3: mod_thread started\n"
        "STEP4: libil2cpp.so found after %d attempts, base=0x%lx\n"
        "STEP5: all hooks installed, all mods ON\n",
        attempts, (unsigned long)base);
    debug_write(buf);

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
