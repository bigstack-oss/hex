// HEX SDK

#ifndef YML_UTIL_H_
#define YML_UTIL_H_

#include <string.h>
#include <yaml.h>
#include <glib.h>

#include <hex/parse.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef gboolean (*TraverselFunc)(GNode *n, gpointer data);

// initialize N-ary tree with name (rootName).
// should call FiniYml() to free N-ary tree
GNode* InitYml(const char *rootName);

// free N-ary tree (cfg)
void FiniYml(GNode *cfg);

// poulate .yml (policyFile) content to N-ary tree (cfg)
int ReadYml(const char *policyFile, GNode *cfg);

// write N-ary tree (cfg) to .yml (policyFile)
int WriteYml(const char *policyFile, GNode *cfg);

// use dot notation to read a map
// use index to access sequence object (start with 1)
// e.g.
//     (MAP)        interfaces
//     (SEQ MAP)    interfaces.1.ipv4
GNode* FindYmlNode(GNode *cfg, const char *path);

// use dot notation to read a value for node
// use index to access sequence object (start with 1)
// e.g.
//     (NODE)        hostname
//     (SEQ NODE)    interfaces.1.ipv4.ipaddr
const char* FindYmlValue(GNode *cfg, const char *path);
const char* FindYmlValueF(GNode *cfg, const char *fmt, ...);

// use dot notation to change a value for node
int UpdateYmlValue(GNode *cfg, const char *path, const char *value);

/* path is NULL means under root */

// add a new node (key, value) under path
int AddYmlNode(GNode *cfg, const char *path, const char *key, const char *value);

// add a new key under path (e.g. seq idx / map)
int AddYmlKey(GNode *cfg, const char *path, const char *key);

// delete all sub nodes under path (include node itself)
int DeleteYmlNode(GNode *cfg, const char *path);

// delete all sub nodes under path (exclude node itself)
int DeleteYmlChildren(GNode *cfg, const char *path);

// get the number of sequence identified by (path) of (cfg)
// return 0 if not found
size_t SizeOfYmlSeq(GNode *cfg, const char *path);

// help function to operate over the N-ary tree
void TraverseYml(GNode *cfg, TraverselFunc func, gpointer data);

// debug function for dumping N-ary tree by giving file path (policyFile)
void DumpYml(const char *policyFile);

// debug function for dumping N-ary tree (cfg)
void DumpYmlNode(GNode *node);

#ifdef __cplusplus
} /* end extern "C" */
#endif

/* C++ only helper functions */
#ifdef __cplusplus

#include <string>

inline void
HexYmlParseInt(int64_t *integer, int64_t min, int64_t max, GNode *cfg, const char *fmt, ...)
{
    char *path = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0)
        return;
    va_end(ap);

    char *value = (char *)FindYmlValue(cfg, path);
    if (HexValidateInt(value, min, max))
        HexParseInt(value, min, max, integer);

    free(path);
}

inline void
HexYmlParseUInt(uint64_t *integer, uint64_t min, uint64_t max, GNode *cfg, const char *fmt, ...)
{
    char *path = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0)
        return;
    va_end(ap);

    char *value = (char *)FindYmlValue(cfg, path);
    if (HexValidateUInt(value, min, max))
        HexParseUInt(value, min, max, integer);

    free(path);
}

inline void
HexYmlParseBool(bool *b, GNode *cfg, const char *fmt, ...)
{
    char *path = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0)
        return;
    va_end(ap);

    char *value = (char *)FindYmlValue(cfg, path);
    if (HexValidateBool(value))
        HexParseBool(value, b);

    free(path);
}

inline void
HexYmlParseString(std::string &str, GNode *cfg, const char *fmt, ...)
{
    char *path = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0)
        return;
    va_end(ap);

    char *value = (char *)FindYmlValue(cfg, path);
    if (value)
        str.assign(value);

    free(path);
}

#endif /* endif __cplusplus */

#endif /* endif CLI_UTIL_H_ */

