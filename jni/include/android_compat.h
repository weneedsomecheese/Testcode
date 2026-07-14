#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#define RTLD_LAZY     0x00001
#define PROT_READ     0x1
#define PROT_WRITE    0x2
#define PROT_EXEC     0x4
#define MAP_PRIVATE   0x02
#define MAP_ANONYMOUS 0x20
#define MAP_FAILED    ((void *)-1)
#define _SC_PAGESIZE  30

typedef int32_t pid_t;
typedef long    off_t;
typedef unsigned long pthread_t;

extern "C" {

void *dlopen(const char *filename, int flags);
void *dlsym(void *handle, const char *symbol);
char *dlerror(void);
int   dlclose(void *handle);

void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
char *strstr(const char *haystack, const char *needle);
size_t strlen(const char *s);

void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);
int   munmap(void *addr, size_t length);
int   mprotect(void *addr, size_t len, int prot);

long  sysconf(int name);
int   usleep(unsigned int usec);

int   pthread_create(pthread_t *thread, const void *attr,
                     void *(*start_routine)(void *), void *arg);
int   pthread_detach(pthread_t thread);

struct dl_phdr_info {
    uintptr_t dlpi_addr;
    const char *dlpi_name;
    const void *dlpi_phdr;
    uint16_t dlpi_phnum;
};

int dl_iterate_phdr(int (*callback)(struct dl_phdr_info *info,
                    size_t size, void *data), void *data);

}
