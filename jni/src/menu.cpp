#include "menu.h"
#include "log.h"

#include <cstring>
#include <pthread.h>

namespace menu {

static constexpr int MAX_TOGGLES = 32;
static constexpr int MAX_SLIDERS = 16;

static bool toggles[MAX_TOGGLES] = {};
static int sliders[MAX_SLIDERS] = {};
static bool visible = false;

static JNIEnv *g_env = nullptr;
static jobject g_context = nullptr;

static jclass g_menu_class = nullptr;
static jmethodID g_show_method = nullptr;
static jmethodID g_hide_method = nullptr;

static const char *MENU_CLASS_SRC = R"(
package com.mod.menu;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;

public class ModMenu {
    private static WindowManager windowManager;
    private static LinearLayout menuLayout;
    private static boolean isShowing = false;

    public static void show(Context context) {
        if (isShowing) return;
        new Handler(Looper.getMainLooper()).post(() -> createMenu(context));
    }

    public static void hide(Context context) {
        if (!isShowing || windowManager == null || menuLayout == null) return;
        new Handler(Looper.getMainLooper()).post(() -> {
            windowManager.removeView(menuLayout);
            isShowing = false;
        });
    }

    private static void createMenu(Context context) {
        windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        menuLayout = new LinearLayout(context);
        menuLayout.setOrientation(LinearLayout.VERTICAL);
        menuLayout.setBackgroundColor(Color.argb(220, 30, 30, 30));
        menuLayout.setPadding(20, 20, 20, 20);

        TextView title = new TextView(context);
        title.setText("IL2CPP Mod Menu");
        title.setTextColor(Color.WHITE);
        title.setTextSize(18);
        title.setGravity(Gravity.CENTER);
        menuLayout.addView(title);

        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
            600, 800,
            WindowManager.LayoutParams.TYPE_APPLICATION,
            WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
            PixelFormat.TRANSLUCENT
        );
        params.gravity = Gravity.TOP | Gravity.LEFT;
        params.x = 100;
        params.y = 200;

        menuLayout.setOnTouchListener(new DragListener(params, windowManager));
        windowManager.addView(menuLayout, params);
        isShowing = true;
    }

    static class DragListener implements View.OnTouchListener {
        private int initialX, initialY;
        private float initialTouchX, initialTouchY;
        private WindowManager.LayoutParams params;
        private WindowManager wm;

        DragListener(WindowManager.LayoutParams p, WindowManager w) {
            params = p;
            wm = w;
        }

        @Override
        public boolean onTouch(View v, MotionEvent event) {
            switch (event.getAction()) {
                case MotionEvent.ACTION_DOWN:
                    initialX = params.x;
                    initialY = params.y;
                    initialTouchX = event.getRawX();
                    initialTouchY = event.getRawY();
                    return true;
                case MotionEvent.ACTION_MOVE:
                    params.x = initialX + (int)(event.getRawX() - initialTouchX);
                    params.y = initialY + (int)(event.getRawY() - initialTouchY);
                    wm.updateViewLayout(v, params);
                    return true;
            }
            return false;
        }
    }
}
)";

void init(JNIEnv *env, jobject context) {
    g_env = env;
    g_context = env->NewGlobalRef(context);
    LOGI("Menu system initialized");
}

void show() {
    visible = true;
    LOGI("Menu shown");
}

void hide() {
    visible = false;
    LOGI("Menu hidden");
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
