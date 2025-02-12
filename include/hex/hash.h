// HEX SDK

#ifndef HEX_HASH_H
#define HEX_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Fowler/Noll/Vo (FNV) Hash function (public domain)

static inline
uint32_t HexHash32(const void *key, int len)
{
    const unsigned char *p = (const unsigned char*)key;
    uint32_t h = 2166136261U;
    int i;
    for (i = 0; i < len; i++)
        h = (h * 16777619U) ^ p[i];

    return h;
}

// FNV xor-folded into 16 bits
static inline
uint16_t HexHash16(const void *key, int len)
{
    uint32_t hash = HexHash32(key, len);
    return (hash >> 16) ^ (hash & 0xffff);
}

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif  /* endif HEX_HASH_H */

