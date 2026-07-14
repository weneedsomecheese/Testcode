#include "il2cpp.h"
#include "log.h"

#include <dlfcn.h>
#include <cstring>
#include <link.h>

namespace il2cpp {

static void *lib_handle = nullptr;
static uintptr_t base_addr = 0;

typedef Il2CppDomain *(*il2cpp_domain_get_t)();
typedef Il2CppThread *(*il2cpp_thread_attach_t)(Il2CppDomain *);
typedef Il2CppAssembly **(*il2cpp_domain_get_assemblies_t)(Il2CppDomain *, size_t *);
typedef Il2CppImage *(*il2cpp_assembly_get_image_t)(const Il2CppAssembly *);
typedef Il2CppClass *(*il2cpp_class_from_name_t)(Il2CppImage *, const char *, const char *);
typedef Il2CppClass *(*il2cpp_class_from_type_t)(const Il2CppType *);
typedef const Il2CppMethodInfo *(*il2cpp_class_get_method_from_name_t)(Il2CppClass *, const char *, int);
typedef Il2CppFieldInfo *(*il2cpp_class_get_field_from_name_t)(Il2CppClass *, const char *);
typedef Il2CppType *(*il2cpp_class_get_type_t)(Il2CppClass *);
typedef void (*il2cpp_field_get_value_t)(Il2CppObject *, Il2CppFieldInfo *, void *);
typedef void (*il2cpp_field_set_value_t)(Il2CppObject *, Il2CppFieldInfo *, void *);
typedef void (*il2cpp_field_static_get_value_t)(Il2CppFieldInfo *, void *);
typedef void (*il2cpp_field_static_set_value_t)(Il2CppFieldInfo *, void *);
typedef void *(*il2cpp_resolve_icall_t)(const char *);
typedef Il2CppString *(*il2cpp_string_new_t)(const char *);
typedef char *(*il2cpp_string_chars_t)(Il2CppString *);

static il2cpp_domain_get_t fn_domain_get;
static il2cpp_thread_attach_t fn_thread_attach;
static il2cpp_domain_get_assemblies_t fn_domain_get_assemblies;
static il2cpp_assembly_get_image_t fn_assembly_get_image;
static il2cpp_class_from_name_t fn_class_from_name;
static il2cpp_class_from_type_t fn_class_from_type;
static il2cpp_class_get_method_from_name_t fn_class_get_method_from_name;
static il2cpp_class_get_field_from_name_t fn_class_get_field_from_name;
static il2cpp_class_get_type_t fn_class_get_type;
static il2cpp_field_get_value_t fn_field_get_value;
static il2cpp_field_set_value_t fn_field_set_value;
static il2cpp_field_static_get_value_t fn_field_static_get_value;
static il2cpp_field_static_set_value_t fn_field_static_set_value;
static il2cpp_resolve_icall_t fn_resolve_icall;
static il2cpp_string_new_t fn_string_new;
static il2cpp_string_chars_t fn_string_chars;

struct callback_data {
    const char *name;
    uintptr_t addr;
};

static int phdr_callback(struct dl_phdr_info *info, size_t, void *data) {
    auto *cbd = static_cast<callback_data *>(data);
    if (info->dlpi_name && strstr(info->dlpi_name, cbd->name)) {
        cbd->addr = info->dlpi_addr;
        return 1;
    }
    return 0;
}

static uintptr_t find_lib_base(const char *lib_name) {
    callback_data cbd = {lib_name, 0};
    dl_iterate_phdr(phdr_callback, &cbd);
    return cbd.addr;
}

#define RESOLVE(handle, name) \
    fn_##name = reinterpret_cast<decltype(fn_##name)>(dlsym(handle, "il2cpp_" #name)); \
    if (!fn_##name) LOGW("Failed to resolve il2cpp_" #name);

bool init(const char *lib_name) {
    lib_handle = dlopen(lib_name, RTLD_LAZY);
    if (!lib_handle) {
        LOGE("Failed to dlopen %s: %s", lib_name, dlerror());
        return false;
    }

    base_addr = find_lib_base(lib_name);
    LOGI("il2cpp base: 0x%X", base_addr);

    RESOLVE(lib_handle, domain_get);
    RESOLVE(lib_handle, thread_attach);
    RESOLVE(lib_handle, domain_get_assemblies);
    RESOLVE(lib_handle, assembly_get_image);
    RESOLVE(lib_handle, class_from_name);
    RESOLVE(lib_handle, class_from_type);
    RESOLVE(lib_handle, class_get_method_from_name);
    RESOLVE(lib_handle, class_get_field_from_name);
    RESOLVE(lib_handle, class_get_type);
    RESOLVE(lib_handle, field_get_value);
    RESOLVE(lib_handle, field_set_value);
    RESOLVE(lib_handle, field_static_get_value);
    RESOLVE(lib_handle, field_static_set_value);
    RESOLVE(lib_handle, resolve_icall);
    RESOLVE(lib_handle, string_new);

    LOGI("il2cpp API initialized");
    return true;
}

Il2CppDomain *domain_get() {
    return fn_domain_get ? fn_domain_get() : nullptr;
}

Il2CppThread *thread_attach(Il2CppDomain *domain) {
    return fn_thread_attach ? fn_thread_attach(domain) : nullptr;
}

Il2CppAssembly **domain_get_assemblies(Il2CppDomain *domain, size_t *count) {
    return fn_domain_get_assemblies ? fn_domain_get_assemblies(domain, count) : nullptr;
}

Il2CppImage *assembly_get_image(const Il2CppAssembly *assembly) {
    return fn_assembly_get_image ? fn_assembly_get_image(assembly) : nullptr;
}

Il2CppClass *class_from_name(Il2CppImage *image, const char *namespaze, const char *name) {
    return fn_class_from_name ? fn_class_from_name(image, namespaze, name) : nullptr;
}

Il2CppClass *class_from_type(const Il2CppType *type) {
    return fn_class_from_type ? fn_class_from_type(type) : nullptr;
}

const Il2CppMethodInfo *class_get_method_from_name(Il2CppClass *klass, const char *name, int argsCount) {
    return fn_class_get_method_from_name ? fn_class_get_method_from_name(klass, name, argsCount) : nullptr;
}

Il2CppFieldInfo *class_get_field_from_name(Il2CppClass *klass, const char *name) {
    return fn_class_get_field_from_name ? fn_class_get_field_from_name(klass, name) : nullptr;
}

Il2CppType *class_get_type(Il2CppClass *klass) {
    return fn_class_get_type ? fn_class_get_type(klass) : nullptr;
}

void field_get_value(Il2CppObject *obj, Il2CppFieldInfo *field, void *value) {
    if (fn_field_get_value) fn_field_get_value(obj, field, value);
}

void field_set_value(Il2CppObject *obj, Il2CppFieldInfo *field, void *value) {
    if (fn_field_set_value) fn_field_set_value(obj, field, value);
}

void field_static_get_value(Il2CppFieldInfo *field, void *value) {
    if (fn_field_static_get_value) fn_field_static_get_value(field, value);
}

void field_static_set_value(Il2CppFieldInfo *field, void *value) {
    if (fn_field_static_set_value) fn_field_static_set_value(field, value);
}

void *resolve_icall(const char *name) {
    return fn_resolve_icall ? fn_resolve_icall(name) : nullptr;
}

Il2CppString *string_new(const char *str) {
    return fn_string_new ? fn_string_new(str) : nullptr;
}

const char *string_to_utf8(Il2CppString *str) {
    if (!fn_string_chars || !str) return "";
    return reinterpret_cast<const char *>(fn_string_chars(str));
}

uintptr_t get_base_address() {
    return base_addr;
}

void *get_method_pointer(const Il2CppMethodInfo *method) {
    if (!method) return nullptr;
    return *reinterpret_cast<void **>(const_cast<Il2CppMethodInfo *>(method));
}

Il2CppClass *find_class(const char *namespaze, const char *name) {
    auto *domain = domain_get();
    if (!domain) return nullptr;

    size_t count = 0;
    auto **assemblies = domain_get_assemblies(domain, &count);
    if (!assemblies) return nullptr;

    for (size_t i = 0; i < count; i++) {
        auto *image = assembly_get_image(assemblies[i]);
        if (!image) continue;

        auto *klass = class_from_name(image, namespaze, name);
        if (klass) return klass;
    }

    LOGW("Class not found: %s.%s", namespaze, name);
    return nullptr;
}

} // namespace il2cpp
