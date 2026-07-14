#pragma once

#include "android_compat.h"

#define LOG_TAG "IL2CPP_MOD"

typedef int (*android_log_print_t)(int prio, const char *tag, const char *fmt, ...);

inline android_log_print_t _get_log_fn() {
    static android_log_print_t fn = nullptr;
    static bool tried = false;
    if (!tried) {
        tried = true;
        void *h = dlopen("liblog.so", RTLD_LAZY);
        if (h) fn = reinterpret_cast<android_log_print_t>(dlsym(h, "__android_log_print"));
    }
    return fn;
}

#define LOGI(...) do { auto _f = _get_log_fn(); if (_f) _f(4, LOG_TAG, __VA_ARGS__); } while(0)
#define LOGD(...) do { auto _f = _get_log_fn(); if (_f) _f(3, LOG_TAG, __VA_ARGS__); } while(0)
#define LOGW(...) do { auto _f = _get_log_fn(); if (_f) _f(5, LOG_TAG, __VA_ARGS__); } while(0)
#define LOGE(...) do { auto _f = _get_log_fn(); if (_f) _f(6, LOG_TAG, __VA_ARGS__); } while(0)
