// HEX SDK

#ifndef HEX_EVENT_IMPL_H
#define HEX_EVENT_IMPL_H

#ifdef __cplusplus

#include <vector>
#include <map>
#include <string>

namespace hex_event {

// Binary blob attribute
struct Blob {
    std::string name;
    size_t size;
    unsigned char* data;
};

// Event Attribute map
typedef std::map<std::string, std::string> NvpMap;

// Binary blob attributes
typedef std::vector<Blob> Blobs;

// Get response list from event data
typedef bool (*GetRespListFunc)(const char*, std::string&);

// converts event data to attribute map (key-value strings) and binary blob (attachments)
typedef bool (*ParseEventFunc)(const char*, NvpMap&, Blobs&);

class Module
{
public:
    Module(const char *name, size_t qsize, GetRespListFunc getresp, ParseEventFunc parse);
    ~Module();
};

class TopTenList
{
public:
    TopTenList(const char* listname);
    ~TopTenList();
};

typedef bool (*SetParamFunc)(const char*, const char*);
typedef bool (*InitFunc)(void);
typedef bool (*CleanupFunc)(void);
typedef void (*TimeoutFunc)(void);  // used by RESPONSE_MODULE_TO macro

// Protoype for function takes attribute map (strings) and executes response
// response is the "instance" part of the form "TYPE:instance"
// source is the name of the event source
typedef bool (*ProcessFunc)(const char* response, const char* source, const NvpMap&, const Blobs&);

class Response
{
public:
    Response(const char *name,
             SetParamFunc setparam,
             ProcessFunc process,
             InitFunc init,
             CleanupFunc cleanup,
             TimeoutFunc timeout_func = 0,
             int timeout = 0);
    ~Response();
};

struct Observes
{
    Observes(const char *resp, const char *prefix, SetParamFunc parse);
};

}; // end namespace hex_event

#endif // __cplusplus

#endif /* endif HEX_EVENT_IMPL_H */

