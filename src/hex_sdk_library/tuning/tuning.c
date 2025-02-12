// HEX SDK

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <hex/tuning.h>

struct HexTuning {
    FILE *stream;
    int line;
    char nameBuf[HEX_TUNING_NAME_MAXLEN + 1];
    char valueBuf[HEX_TUNING_VALUE_MAXLEN + 1];
};

HexTuning_t
HexTuningAlloc(FILE *stream)
{
    if (stream == NULL)
        return NULL;

    HexTuning_t tuning = (HexTuning_t) malloc(sizeof(struct HexTuning));
    if (!tuning)
        return NULL;

    tuning->stream = stream;
    tuning->line = 0;
    memset(tuning->nameBuf, 0, sizeof(tuning->nameBuf));
    memset(tuning->valueBuf, 0, sizeof(tuning->valueBuf));
    return tuning;
}

void
HexTuningRelease(HexTuning_t tuning)
{
    if (tuning)
        free(tuning);
}

// getc() with line counting
static inline int
Getc(HexTuning_t tuning)
{
    int c = getc(tuning->stream);
    if (c == '\n' || c == EOF)
        ++tuning->line;
    return c;
}

// ungetc() with line counting
static inline void
Ungetc(HexTuning_t tuning, int c)
{
    if (c == '\n' || c == EOF)
        --tuning->line; // COV_IGNORE: not reachable with current parser, but can't hurt
    ungetc(c, tuning->stream);
}

// Set stream to null on error to prevent further reads
static inline int
Error(HexTuning_t tuning, int error)
{
    tuning->stream = NULL;
    return error;
}

int
HexTuningParseLineWithD(HexTuning_t tuning, const char **name, const char **value, const char d)
{
    if (tuning == NULL || tuning->stream == NULL)
        return HEX_TUNING_ERROR;

    *name = tuning->nameBuf;
    tuning->nameBuf[0] = '\0';

    *value = tuning->valueBuf;
    tuning->valueBuf[0] = '\0';

    // Skip over blank lines, leading whitespace, and comments
    int c;
    while ((c = Getc(tuning)) != EOF) {
        if (c == '#') {
            while ((c = Getc(tuning)) != EOF && c != '\n')
                ;
            if (c == EOF)
                return Error(tuning, HEX_TUNING_EOF);
            assert(c == '\n');
        } else if (!isspace(c)) {
            Ungetc(tuning, c);
            break;
        }
    }
    if (c == EOF)
        return Error(tuning, HEX_TUNING_EOF);

    // Parse name
    size_t n = 0;
    char *p = tuning->nameBuf;
    while ((c = Getc(tuning)) != EOF) {
        if (c == '\n')
            // don't nullify file stream for caller to parse themselves
            return HEX_TUNING_MALFORMED;
        else if (isspace(c) || c == d) {
            Ungetc(tuning, c);
            break;
        }
        if (n++ < HEX_TUNING_NAME_MAXLEN + 1)
            *p++ = c;
        else
            return Error(tuning, HEX_TUNING_EXCEEDED);
    }
    if (c == EOF)
        return Error(tuning, HEX_TUNING_MALFORMED);

    // Check for only delimiter with no name
    if (*name == p)
        return Error(tuning, HEX_TUNING_MALFORMED);

    // Null terminate name
    if (n++ < HEX_TUNING_NAME_MAXLEN + 1)
        *p++ = '\0';
    else
        return Error(tuning, HEX_TUNING_EXCEEDED);

    // Skip over spaces up to and including delimiter
    while ((c = Getc(tuning)) != EOF) {
        if (c == d)
            break;
        else if (c == '\n' || !isspace(c))
            return Error(tuning, HEX_TUNING_MALFORMED);
    }
    if (c == EOF)
        return Error(tuning, HEX_TUNING_MALFORMED);

    // Skip over spaces before value
    while ((c = Getc(tuning)) != EOF) {
        if (c == '\n')
            break;
        if (!isspace(c)) {
            Ungetc(tuning, c);
            break;
        }
    }
    if (c == '\n' || c == EOF) {
        // End of line or EOF reached, value is empty
        return HEX_TUNING_SUCCESS;
    }

    // Parse value
    n = 0;
    p = tuning->valueBuf;

    // Check if value is surrounded by quotes
    if ((c = Getc(tuning)) == '"') {

        while ((c = Getc(tuning)) != EOF) {
            // A single quote (") terminates the value
            // Two quotes ("") inserts a single quote into the value
            if (c == '"') {
                if ((c = Getc(tuning)) != '"') {
                    Ungetc(tuning, c);
                    break;
                }
            }
            if (n++ < HEX_TUNING_VALUE_MAXLEN + 1)
                *p++ = c;
            else
                return Error(tuning, HEX_TUNING_EXCEEDED);
        }
        if (c == EOF)
            return Error(tuning, HEX_TUNING_MALFORMED);

        // Skip over spaces after quoted value
        while ((c = Getc(tuning)) != EOF) {
            if (c == '\n')
                break;
            if (!isspace(c))
                return Error(tuning, HEX_TUNING_MALFORMED);
        }

        // Null terminate value
        if (n++ < HEX_TUNING_VALUE_MAXLEN + 1)
            *p++ = '\0';
        else
            return Error(tuning, HEX_TUNING_EXCEEDED);

    } else {

        Ungetc(tuning, c);

        // Remember last non-space character so we can strip trailing space later
        char *lastNonSpace = p;
        while ((c = Getc(tuning)) != EOF && c != '\n') {
            if (!isspace(c))
                lastNonSpace = p;
            if (n++ < HEX_TUNING_VALUE_MAXLEN + 1)
                *p++ = c;
            else
                return Error(tuning, HEX_TUNING_EXCEEDED);
        }

        // Null terminate value
        if (++lastNonSpace < p || n < HEX_TUNING_VALUE_MAXLEN + 1)
            *lastNonSpace = '\0';
        else
            return Error(tuning, HEX_TUNING_EXCEEDED);
    }

    return HEX_TUNING_SUCCESS;
}

int
HexTuningParseLine(HexTuning_t tuning, const char **name, const char **value)
{
    return HexTuningParseLineWithD(tuning, name, value, '=');
}

int
HexTuningCurrLine(HexTuning_t tuning)
{
    if (!tuning)
        return 0;

    return tuning->line;
}

