#include "android_compat.h"

__attribute__((constructor))
static void load_mod() {
    dlopen("libmodmenu.so", RTLD_LAZY);
}
