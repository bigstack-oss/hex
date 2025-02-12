// HEX SDK

#ifndef HEX_CRASH_H
#define HEX_CRASH_H

#ifdef __cplusplus
extern "C" {
#endif

int HexCrashInit(const char* name);

typedef void (*HexCrashCallback)(void* data, void* context);

// prepend a new function for being called before pre-defined signal handler
int HexCrashInitPrepend(const char* name, HexCrashCallback func, void* data);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_CRASH_H */

