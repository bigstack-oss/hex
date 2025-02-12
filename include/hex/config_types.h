// HEX SDK

#ifndef HEX_CONFIGTYPES_H
#define HEX_CONFIGTYPES_H

#include <hex/parse.h>

#ifdef __cplusplus

#include <fstream>
#include <map>
#include <vector>
#include <limits.h>

template <class C, class T>
class ConfigTypeArray
{
private:
    std::vector<C> m_array;
    T m_def;
public:
    ConfigTypeArray(const T& def) : m_def(def) { }

    bool parse(size_t key, const char *value, bool isNew)
    {
        C config(m_def);
        if (!config.parse(value, isNew))
            return false;

        if (key >= m_array.size()) {
            m_array.resize(key + 1);
        }

        return m_array[key].parse(value, isNew);
    }

    bool modified() const
    {
        bool m = false;
        for (auto it = m_array.begin(); it != m_array.end(); ++it) {
            if (it->modified()) {
                m = true;
                break;
            }
        }
        return m;
    }

    const T oldValue(size_t key) const { return m_array.at(key).oldValue(); }
    const T newValue(size_t key) const { return m_array.at(key).newValue(); }
    auto begin() const { return m_array.begin(); }
    auto end() const { return m_array.end(); }
    size_t size() const { return m_array.size(); }
    void resize(size_t n) { m_array.resize(n); }
};

template <class C, class T>
class ConfigTypeMap
{
private:
    std::map<std::string, C> m_map;
    T m_def;
public:
    ConfigTypeMap(const T& def) : m_def(def) {}

    bool parse(const char *key, const char *value, bool isNew)
    {
        C config(m_def);
        if (!config.parse(value, isNew))
            return false;

        auto i = m_map.find(key);
        if (i == m_map.end())
            m_map.insert(std::pair<std::string, C>(key, config));
        else
            m_map[key].parse(value, isNew);

        return true;
    }

    bool modified() const
    {
        bool m = false;
        for (auto it = m_map.begin(); it != m_map.end(); ++it) {
            if (it->second.modified())
                m = true;
        }
        return m;
    }

    const T oldValue(const char *key) const { return m_map.at(key).oldValue(); }
    const T newValue(const char *key) const { return m_map.at(key).newValue(); }
    auto find(const std::string& key) const { return m_map.find(key); }
    auto find(const char *key) const { return m_map.find(key); }
    auto begin() const { return m_map.begin(); }
    auto end() const { return m_map.end(); }
    size_t size() const { return m_map.size(); }
};

template <class C, class T>
class ConfigRangeArray
{
private:
    std::vector<C> m_array;
    T m_def, m_min, m_max;
public:
    ConfigRangeArray(const T& def, const T& min, const T& max) : m_def(def), m_min(min), m_max(max) { }

    bool parse(size_t key, const char *value, bool isNew)
    {
        C config(m_def, m_min, m_max);
        if (!config.parse(value, isNew))
            return false;

        if (key >= m_array.size()) {
            m_array.resize(key + 1);
        }

        return m_array[key].parse(value, isNew);
    }

    bool modified() const
    {
        bool m = false;
        for (auto it = m_array.begin(); it != m_array.end(); ++it) {
            if (it->modified()) {
                m = true;
                break;
            }
        }
        return m;
    }

    const T oldValue(size_t key) const { return m_array.at(key).oldValue(); }
    const T newValue(size_t key) const { return m_array.at(key).newValue(); }
    auto begin() const { return m_array.begin(); }
    auto end() const { return m_array.end(); }
    size_t size() const { return m_array.size(); }
    void resize(size_t n) { m_array.resize(n); }
};


template <class C, class T>
class ConfigRangeMap
{
private:
    std::map<std::string, C> m_map;
    T m_def, m_min, m_max;
public:
    ConfigRangeMap(const T& def, const T& min, const T& max) : m_def(def), m_min(min), m_max(max) { }

    bool parse(const char *key, const char *value, bool isNew)
    {
        C config(m_def, m_min, m_max);
        if (!config.parse(value, isNew))
            return false;

        auto i = m_map.find(key);
        if (i == m_map.end())
            m_map.insert(std::pair<std::string, C>(key, config));
        else
            m_map[key].parse(value, isNew);

        return true;
    }

    bool modified() const
    {
        bool m = false;
        for (auto it = m_map.begin(); it != m_map.end(); ++it) {
            if (it->second.modified())
                m = true;
        }
        return m;
    }

    const T oldValue(const char *key) const { return m_map.at(key).oldValue(); }
    const T newValue(const char *key) const { return m_map.at(key).newValue(); }
    auto find(const std::string& key) const { return m_map.find(key); }
    auto find(const char *key) const { return m_map.find(key); }
    auto begin() const { return m_map.begin(); }
    auto end() const { return m_map.end(); }
    size_t size() const { return m_map.size(); }
};

class ConfigString
{
private:
    std::string m_old, m_new;
public:
    ConfigString() : m_old(), m_new() { }
    ConfigString(const std::string& def)
        : m_old(def), m_new(def) { }
    ConfigString(const char *def)
        : m_old(def), m_new(def) { }

    ConfigString& operator=(const char* rhs)
    {
        m_new = rhs;
        return *this;
    }

    bool parse(const char *value, bool isNew)
    {
        if (isNew) m_new = value;
        else m_old = value;
        return true;
    }

    bool modified() const { return m_old !=  m_new; }
    const std::string& oldValue() const { return m_old; }
    const std::string& newValue() const { return  m_new; }
    const char* c_str() const { return  m_new.c_str(); }
    bool empty() const { return  m_new.empty(); }
    size_t length() const { return  m_new.length(); }
    operator std::string() const { return m_new; }
};

typedef ConfigTypeArray<ConfigString, std::string> ConfigStringArray;
typedef ConfigTypeMap<ConfigString, std::string> ConfigStringMap;

class ConfigBool
{
private:
    bool m_old, m_new;
public:
    ConfigBool() : m_old(false), m_new(false) { }
    ConfigBool(bool def)
        : m_old(def), m_new(def) { }

    ConfigBool& operator=(bool rhs)
    {
        m_new = rhs;
        return *this;
    }

    bool parse(const char *value, bool isNew)
    {
        bool valueAsBool;
        if (!HexParseBool(value, &valueAsBool))
            return false;
        if (isNew) m_new = valueAsBool;
        else m_old = valueAsBool;
        return true;
    }

    bool modified() const { return m_old !=  m_new; }
    bool oldValue() const { return m_old; }
    bool newValue() const { return  m_new; }
    operator bool() const { return m_new; }
};

typedef ConfigTypeArray<ConfigBool, bool> ConfigBoolArray;
typedef ConfigTypeMap<ConfigBool, bool> ConfigBoolMap;

class ConfigInt
{
private:
    int m_old, m_new;
    int m_min, m_max;
public:
    ConfigInt()
        : m_old(0), m_new(0),
          m_min(INT_MIN), m_max(INT_MAX) { }
    ConfigInt(int def, int min, int max)
        : m_old(def), m_new(def),
          m_min(min), m_max(max) { }

    ConfigInt& operator=(int rhs)
    {
        m_new = rhs;
        return *this;
    }

    bool parse(const char *value, bool isNew)
    {
        int64_t valueAsInt;
        if (!HexParseInt(value, m_min, m_max, &valueAsInt))
            return false;
        if (isNew) m_new = valueAsInt;
        else m_old = valueAsInt;
        return true;
    }

    bool modified() const { return m_old != m_new; }
    int oldValue() const { return m_old; }
    int newValue() const { return  m_new; }
    operator int() const { return m_new; }
};

typedef ConfigRangeArray<ConfigInt, int> ConfigIntArray;
typedef ConfigRangeMap<ConfigInt, int> ConfigIntMap;

class ConfigUInt
{
private:
    unsigned m_old, m_new;
    unsigned m_min, m_max;
public:
    ConfigUInt()
        : m_old(0), m_new(0),
          m_min(0), m_max(UINT_MAX) { }
    ConfigUInt(unsigned def, unsigned min, unsigned max)
        : m_old(def), m_new(def),
          m_min(min), m_max(max) { }

    ConfigUInt& operator=(unsigned rhs)
    {
        m_new = rhs;
        return *this;
    }

    bool parse(const char *value, bool isNew)
    {
        uint64_t valueAsUInt;
        if (!HexParseUInt(value, m_min, m_max, &valueAsUInt))
            return false;
        if (isNew) m_new = valueAsUInt;
        else m_old = valueAsUInt;
        return true;
    }

    bool modified() const { return m_old !=  m_new; }
    unsigned oldValue() const { return m_old; }
    unsigned newValue() const { return  m_new; }
    operator unsigned int() const { return m_new; }
};

typedef ConfigRangeArray<ConfigUInt, unsigned> ConfigUIntArray;
typedef ConfigRangeMap<ConfigUInt, unsigned> ConfigUIntMap;

#endif /* endif __cplusplus */

#endif /* endif HEX_CONFIGYTPES_H */

