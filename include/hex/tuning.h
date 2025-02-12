// HEX SDK

#ifndef HEX_TUNING_H
#define HEX_TUNING_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Opaque type
typedef struct HexTuning *HexTuning_t;

HexTuning_t HexTuningAlloc(FILE *stream);

void HexTuningRelease(HexTuning_t tuning);

enum {
    HEX_TUNING_EOF = -1,
    HEX_TUNING_SUCCESS = 0,
    HEX_TUNING_EXCEEDED,
    HEX_TUNING_MALFORMED,
    HEX_TUNING_ERROR
};

// Maximum length of name and value fields
enum {
    HEX_TUNING_NAME_MAXLEN = 256 - 1,
    HEX_TUNING_VALUE_MAXLEN = 32 * 1024 - 1,
};

int HexTuningParseLine(HexTuning_t tuning, const char **name, const char **value);
int HexTuningParseLineWithD(HexTuning_t tuning, const char **name, const char **value, const char dlmtr);
int HexTuningCurrLine(HexTuning_t tuning);

bool HexMatchPrefix(const char *name, const char *prefix, const char **p);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* ifndef HEX_TUNING_H */
