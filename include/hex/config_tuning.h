// HEX SDK

#ifndef HEX_CONFIGTUNING_H
#define HEX_CONFIGTUNING_H

#ifdef __cplusplus

#include <unordered_map>
#include <string>

#include <hex/log.h>
#include <hex/config_types.h>

enum TuneStatus {
    TUNE_OK,
    TUNE_INVALID_NAME,
    TUNE_INVALID_VALUE,
    TUNE_INVALID_IDX,
};

struct Tune
{
    enum NameType {
        TUNE_P, // "Plain", i.e. no embedded variables
        TUNE_I, // embedded int, e.g. foo.9.bar
        TUNE_S, // string, e.g. foo.MY_STRING.bar
        // TUNE_SI, // string then int, e.g. foo.MY_STRING.bar.9
        // TUNE_IS, // int then string, e.g. foo.9.MYSTRING
    };
    NameType nameType;
    const char* format;

    union {
        int ni;
        const char* ns;
        //...
    };

    Tune(NameType t, const char* f): nameType(t), format(f) {}

    virtual TuneStatus ParseValue(const char* value, bool isNew) = 0;
    virtual bool IsModified() = 0;
};

struct TuningBool: public Tune, public ConfigBool
{
    TuningBool(int def, const char* fmt):
        Tune(Tune::TUNE_P, fmt),
        ConfigBool(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }

    TuningBool& operator=(bool rhs)
    {
        ConfigBool::operator=(rhs);
        return *this;
    }
};

// An array of ConfigBools, indexed by an int
struct TuningBoolArray: public Tune, public ConfigBoolArray
{
    TuningBoolArray(bool def, const char* fmt):
        Tune(Tune::TUNE_I, fmt),
        ConfigBoolArray(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ni, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

// A map of ConfigBools, indexed by a key
struct TuningBoolMap: public Tune, public ConfigBoolMap
{
    TuningBoolMap(bool def, const char* fmt):
        Tune(Tune::TUNE_S, fmt),
        ConfigBoolMap(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ns, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningInt: public Tune, public ConfigInt
{
    TuningInt(int def, int min, int max, const char* fmt):
        Tune(Tune::TUNE_P, fmt),
        ConfigInt(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }

    TuningInt& operator=(int rhs)
    {
        ConfigInt::operator=(rhs);
        return *this;
    }
};

struct TuningIntArray: public Tune, public ConfigIntArray
{
    TuningIntArray(int def, int min, int max, const char* fmt):
        Tune(Tune::TUNE_I, fmt),
        ConfigIntArray(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ni, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningIntMap: public Tune, public ConfigIntMap
{
    TuningIntMap(int def, int min, int max, const char* fmt):
        Tune(Tune::TUNE_S, fmt),
        ConfigIntMap(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ns, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningUInt: public Tune, public ConfigUInt
{
    TuningUInt(unsigned def, unsigned min, unsigned max, const char* fmt):
        Tune(Tune::TUNE_P, fmt),
        ConfigUInt(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }

    TuningUInt& operator=(int rhs)
    {
        ConfigUInt::operator=(rhs);
        return *this;
    }
};

struct TuningUIntArray: public Tune, public ConfigUIntArray
{
    TuningUIntArray(unsigned def, unsigned min, unsigned max, const char* fmt):
        Tune(Tune::TUNE_I, fmt),
        ConfigUIntArray(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ni, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningUIntMap: public Tune, public ConfigUIntMap
{
    TuningUIntMap(unsigned def, unsigned min, unsigned max, const char* fmt):
        Tune(Tune::TUNE_S, fmt),
        ConfigUIntMap(def, min, max)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ns, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningString: public Tune, public ConfigString
{
    TuningString(const char* def, const char* fmt, ValidateType type):
        Tune(Tune::TUNE_P, fmt),
        ConfigString(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }

    TuningString& operator=(const char* rhs)
    {
        ConfigString::operator=(rhs);
        return *this;
    }
};

struct TuningStringArray: public Tune, public ConfigStringArray
{
    TuningStringArray(const char* def, const char* fmt, ValidateType type):
        Tune(Tune::TUNE_I, fmt),
        ConfigStringArray(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ni, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

struct TuningStringMap: public Tune, public ConfigStringMap
{
    TuningStringMap(const char* def, const char* fmt, ValidateType type):
        Tune(Tune::TUNE_S, fmt),
        ConfigStringMap(def)
    {}

    virtual TuneStatus ParseValue(const char* value, bool isNew)
    {
        if (!parse(ns, value, isNew)) {
            return TUNE_INVALID_VALUE;
        }
        return TUNE_OK;
    }

    virtual bool IsModified()
    {
        return modified();
    }
};

#define MAX_TUNING_MAP  10

static std::unordered_map<std::string, Tune*> s_tunes[MAX_TUNING_MAP];

struct TuneMapping {
    TuneMapping(std::unordered_map<std::string, Tune*> *tunes, const char* format, Tune* t, int idx = 0)
    {
        if (idx >= 0 && idx < MAX_TUNING_MAP)
            tunes[idx].insert(std::make_pair(format, t));
    }
};

#define PARSE_TUNING_X_INT(var, spec, idx) \
    CONFIG_TUNING_SPEC_INT(spec); \
    static TuningInt var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_INT_ARRAY(var, spec, idx) \
    CONFIG_TUNING_SPEC_INT(spec); \
    static TuningIntArray var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_INT_MAP(var, spec, idx) \
    CONFIG_TUNING_SPEC_INT(spec); \
    static TuningIntMap var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_UINT(var, spec, idx) \
    CONFIG_TUNING_SPEC_UINT(spec); \
    static TuningUInt var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_UINT_ARRAY(var, spec, idx) \
    CONFIG_TUNING_SPEC_UINT(spec); \
    static TuningUIntArray var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_UINT_MAP(var, spec, idx) \
    CONFIG_TUNING_SPEC_UINT(spec); \
    static TuningUIntMap var(spec.def, spec.min, spec.max, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_BOOL(var, spec, idx) \
    CONFIG_TUNING_SPEC_BOOL(spec); \
    static TuningBool var(spec.def, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_BOOL_ARRAY(var, spec, idx) \
    CONFIG_TUNING_SPEC_BOOL(spec); \
    static TuningBoolArray var(spec.def, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_BOOL_MAP(var, spec, idx) \
    CONFIG_TUNING_SPEC_BOOL(spec); \
    static TuningBoolMap var(spec.def, spec.format.c_str()); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_STR(var, spec, idx) \
    CONFIG_TUNING_SPEC_STR(spec); \
    static TuningString var(spec.def.c_str(), spec.format.c_str(), spec.type); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_STR_ARRAY(var, spec, idx) \
    CONFIG_TUNING_SPEC_STR(spec); \
    static TuningStringArray var(spec.def.c_str(), spec.format.c_str(), spec.type); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)

#define PARSE_TUNING_X_STR_MAP(var, spec, idx) \
    CONFIG_TUNING_SPEC_STR(spec); \
    static TuningStringMap var(spec.def.c_str(), spec.format.c_str(), spec.type); \
    static TuneMapping tune_mapping_##var(s_tunes, spec.format.c_str(), &var, idx)


#define PARSE_TUNING_INT(var, spec)         PARSE_TUNING_X_INT(var, spec, 0)
#define PARSE_TUNING_INT_ARRAY(var, spec)   PARSE_TUNING_X_INT_ARRAY(var, spec, 0)
#define PARSE_TUNING_INT_MAP(var, spec)     PARSE_TUNING_X_INT_MAP(var, spec, 0)
#define PARSE_TUNING_UINT(var, spec)        PARSE_TUNING_X_UINT(var, spec, 0)
#define PARSE_TUNING_UINT_ARRAY(var, spec)  PARSE_TUNING_X_UINT_ARRAY(var, spec, 0)
#define PARSE_TUNING_UINT_MAP(var, spec)    PARSE_TUNING_X_UINT_MAP(var, spec, 0)
#define PARSE_TUNING_BOOL(var, spec)        PARSE_TUNING_X_BOOL(var, spec, 0)
#define PARSE_TUNING_BOOL_ARRAY(var, spec)  PARSE_TUNING_X_BOOL_ARRAY(var, spec, 0)
#define PARSE_TUNING_BOOL_MAP(var, spec)    PARSE_TUNING_X_BOOL_MAP(var, spec, 0)
#define PARSE_TUNING_STR(var, spec)         PARSE_TUNING_X_STR(var, spec, 0)
#define PARSE_TUNING_STR_ARRAY(var, spec)   PARSE_TUNING_X_STR_ARRAY(var, spec, 0)
#define PARSE_TUNING_STR_MAP(var, spec)     PARSE_TUNING_X_STR_MAP(var, spec, 0)

static inline TuneStatus
ParseTune(const char* n, const char* v, bool isNew, int idx = 0) //TODO: move to .cpp file
{
    if (idx < 0 || idx >= MAX_TUNING_MAP)
        return TUNE_INVALID_IDX;

    TuneStatus r = TUNE_INVALID_NAME;
    for (auto it = s_tunes[idx].begin(); r == TUNE_INVALID_NAME && it != s_tunes[idx].end(); ++it) {
        Tune* t = it->second;
        const char* format = it->first.c_str();
        HexLogDebugN(RRA, "Checking %s against %s...", n, format);
        switch (t->nameType) {
            case Tune::TUNE_P: {
                if (strcmp(n, format) == 0) {
                    r = t->ParseValue(v, isNew);
                }
                break;
            }
            case Tune::TUNE_I: {
                int arg, pos = 0;
                char fmt[1024];
                snprintf(fmt, sizeof(fmt), "%s%%n", format);
                if (sscanf(n, fmt, &arg, &pos) == 1 && pos == (int)strlen(n)) {
                    t->ni = arg;
                    r = t->ParseValue(v, isNew);
                }
                break;
            }
            case Tune::TUNE_S: {
                char arg[80], fmt[1024];
                int pos = 0;
                snprintf(fmt, sizeof(fmt), "%s%%n", format);
                if (sscanf(n, fmt, &arg, &pos) == 1 && pos == (int)strlen(n)) {
                    t->ns = arg;
                    r = t->ParseValue(v, isNew);
                }
                break;
            }
        }
    }
    return r;
}

static inline bool
IsModifiedTune(int idx = 0) //TODO: move to .cpp file
{
    if (idx < 0 || idx >= MAX_TUNING_MAP)
        return false;

    for (auto it = s_tunes[idx].begin(); it != s_tunes[idx].end(); ++it) {
        Tune* t = it->second;
        if (t->IsModified())
            return true;
    }
    return false;
}

#endif /* endif __cplusplus */

#endif /* endif HEX_CONFIGTUNING_H */

