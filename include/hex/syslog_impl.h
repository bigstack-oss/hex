// HEX SDK

#ifndef HEX_SYSLOGIMPL_H
#define HEX_SYSLOGIMPL_H

#include <stdio.h>  // size_t
#include <stdint.h> // uint8_t

#define LOG_MAX_LEN 1024
#define HEX_SYSLOGD_QSIZE (LOG_MAX_LEN * 64)

typedef struct SyslogMsg_
{
    char raw[LOG_MAX_LEN + 2]; // Save room for newline and null
    char* timestamp;
    char* tag;
    size_t tagLen;
    char* content;
    uint8_t facility;
    uint8_t severity;
} SyslogMsg;

#ifdef __cplusplus

#include <map>
#include <string>

namespace hex_syslog {

// Protoype for function that (re)initializes a module
typedef bool (*InitFunc)();

// Protoype for function that passes a parameter to a module
typedef bool (*SetParamFunc)(const char*, const char*);

// Protoype for function that parses a syslog message
typedef bool (*ParseFunc)(const SyslogMsg&);

// Protoype for function that handles regex matches
// Args can be modified by this function
typedef std::map<std::string, std::string> NvpMap;
typedef bool (*MatchFunc)(const std::string& id, const SyslogMsg&, NvpMap& args);

class Module {
public:
    Module(std::string name, InitFunc init, SetParamFunc set_param, ParseFunc parse);
~Module();
};

class ModuleEventRegex
{
public:
    ModuleEventRegex(
        const char *eventid,
        const char *eventsource,
        const char *eventregex,
        MatchFunc match = NULL);
    ~ModuleEventRegex();
};

}; // end namespace hex_syslog

#endif // __cplusplus

#endif /* endif HEX_SYSLOGIMPL_H */

