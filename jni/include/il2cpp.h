#pragma once

#include "android_compat.h"

typedef void Il2CppDomain;
typedef void Il2CppAssembly;
typedef void Il2CppImage;
typedef void Il2CppClass;
typedef void Il2CppMethodInfo;
typedef void Il2CppFieldInfo;
typedef void Il2CppType;
typedef void Il2CppObject;
typedef void Il2CppString;
typedef void Il2CppArray;
typedef void Il2CppThread;

struct Il2CppArraySize {
    void *obj;
    void *bounds;
    uintptr_t max_length;
    void *vector[0];
};

namespace il2cpp {

bool init(const char *lib_name = "libil2cpp.so");

Il2CppDomain *domain_get();
Il2CppThread *thread_attach(Il2CppDomain *domain);

Il2CppAssembly **domain_get_assemblies(Il2CppDomain *domain, size_t *count);

Il2CppImage *assembly_get_image(const Il2CppAssembly *assembly);

Il2CppClass *class_from_name(Il2CppImage *image, const char *namespaze, const char *name);
Il2CppClass *class_from_type(const Il2CppType *type);

const Il2CppMethodInfo *class_get_method_from_name(Il2CppClass *klass, const char *name, int argsCount);
Il2CppFieldInfo *class_get_field_from_name(Il2CppClass *klass, const char *name);
Il2CppType *class_get_type(Il2CppClass *klass);

void field_get_value(Il2CppObject *obj, Il2CppFieldInfo *field, void *value);
void field_set_value(Il2CppObject *obj, Il2CppFieldInfo *field, void *value);
void field_static_get_value(Il2CppFieldInfo *field, void *value);
void field_static_set_value(Il2CppFieldInfo *field, void *value);

void *resolve_icall(const char *name);

Il2CppString *string_new(const char *str);
const char *string_to_utf8(Il2CppString *str);

uintptr_t get_base_address();
void *get_method_pointer(const Il2CppMethodInfo *method);

Il2CppClass *find_class(const char *namespaze, const char *name);

} // namespace il2cpp
