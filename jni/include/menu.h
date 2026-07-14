#pragma once

namespace menu {

void init();
void show();
void hide();
bool is_visible();

bool get_toggle(int index);
void set_toggle(int index, bool value);
int get_slider(int index);
void set_slider(int index, int value);

} // namespace menu
