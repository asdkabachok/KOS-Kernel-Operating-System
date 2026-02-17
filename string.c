// string.c — String and memory operations FIXED
#include "kernel.h"

void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

void* memcpy(void* d, const void* s, size_t n) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;
    while (n--) *dst++ = *src++;
    return d;
}

void* memmove(void* d, const void* s, size_t n) {
    uint8_t* dst = (uint8_t*)d;
    const uint8_t* src = (const uint8_t*)s;
    if (dst < src) {
        while (n--) *dst++ = *src++;
    } else {
        dst += n;
        src += n;
        while (n--) *--dst = *--src;
    }
    return d;
}

size_t strlen(const char* s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && *s1 == *s2) {
        s1++;
        s2++;
        n--;
    }
    return n ? (unsigned char)*s1 - (unsigned char)*s2 : 0;
}

char* strcpy(char* d, const char* s) {
    char* orig = d;
    while ((*d++ = *s++));
    return orig;
}

char* strncpy(char* d, const char* s, size_t n) {
    char* orig = d;
    while (n && (*d++ = *s++)) n--;
    while (n--) *d++ = 0;
    return orig;
}

// Additional useful string functions
int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}

void* memchr(const void* s, int c, size_t n) {
    const uint8_t* p = (const uint8_t*)s;
    while (n--) {
        if (*p == (uint8_t)c) return (void*)p;
        p++;
    }
    return NULL;
}

char* strchr(const char* s, int c) {
    while (*s) {
        if (*s == (char)c) return (char*)s;
        s++;
    }
    return NULL;
}

char* strcat(char* d, const char* s) {
    char* orig = d;
    while (*d) d++;
    while ((*d++ = *s++));
    return orig;
}

char* strncat(char* d, const char* s, size_t n) {
    char* orig = d;
    while (*d) d++;
    while (n-- && *s) *d++ = *s++;
    *d = 0;
    return orig;
}

size_t strspn(const char* s, const char* accept) {
    size_t n = 0;
    while (*s && strchr(accept, *s)) {
        s++;
        n++;
    }
    return n;
}

size_t strcspn(const char* s, const char* reject) {
    size_t n = 0;
    while (*s) {
        if (strchr(reject, *s)) break;
        s++;
        n++;
    }
    return n;
}

char* strstr(const char* haystack, const char* needle) {
    size_t hlen = strlen(haystack);
    size_t nlen = strlen(needle);
    if (hlen >= nlen) {
        for (size_t i = 0; i <= hlen - nlen; i++) {
            if (memcmp(haystack + i, needle, nlen) == 0) {
                return (char*)haystack + i;
            }
        }
    }
    return NULL;
}
