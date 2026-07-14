#pragma once

#include <cstdarg>
#include <cstdio>
#include <dlfcn.h>

#define LOG_TAG "IL2CPP_MOD"

typedef int (*android_log_print_t)(int prio, const char *tag, const char *fmt, ...);
static android_log_print_t _log_print = nullptr;

enum {
    _ANDROID_LOG_DEBUG = 3,
    _ANDROID_LOG_INFO  = 4,
    _ANDROID_LOG_WARN  = 5,
    _ANDROID_LOG_ERROR = 6,
};

__attribute__((constructor))
static void _init_log() {
    void *h = dlopen("liblog.so", RTLD_LAZY);
    if (h) _log_print = reinterpret_cast<android_log_print_t>(dlsym(h, "__android_log_print"));
}

#define LOGI(...) do { if (_log_print) _log_print(_ANDROID_LOG_INFO,  LOG_TAG, __VA_ARGS__); } while(0)
#define LOGD(...) do { if (_log_print) _log_print(_ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__); } while(0)
#define LOGW(...) do { if (_log_print) _log_print(_ANDROID_LOG_WARN,  LOG_TAG, __VA_ARGS__); } while(0)
#define LOGE(...) do { if (_log_print) _log_print(_ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__); } while(0)
