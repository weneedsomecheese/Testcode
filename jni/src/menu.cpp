#include "menu.h"
#include "log.h"

namespace menu {

static constexpr int MAX_TOGGLES = 32;
static constexpr int MAX_SLIDERS = 16;

static bool toggles[MAX_TOGGLES] = {};
static int sliders[MAX_SLIDERS] = {};
static bool visible = false;

void init() {
    LOGI("Menu system initialized");
}

void show() {
    visible = true;
}

void hide() {
    visible = false;
}

bool is_visible() {
    return visible;
}

bool get_toggle(int index) {
    if (index < 0 || index >= MAX_TOGGLES) return false;
    return toggles[index];
}

void set_toggle(int index, bool value) {
    if (index < 0 || index >= MAX_TOGGLES) return;
    toggles[index] = value;
}

int get_slider(int index) {
    if (index < 0 || index >= MAX_SLIDERS) return 0;
    return sliders[index];
}

void set_slider(int index, int value) {
    if (index < 0 || index >= MAX_SLIDERS) return;
    sliders[index] = value;
}

} // namespace menu
