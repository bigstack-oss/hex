// HEX SDK

#ifndef HEX_EVENT_MODULE_H
#define HEX_EVENT_MODULE_H

#ifdef __cplusplus

#include <hex/hex_impl.h>
#include <hex/event_impl.h>

/** EVENT_MODULE defines the representation of event types: system, network, etc **/

#define EVENT_MODULE(name, qsize, getResp, parse) \
    static hex_event::Module HEX_CAT(s_module_,__LINE__)(#name, qsize, getResp, parse)

/** RESP_MODULE defines ways of handling events: snmp, email, remote, db, etc **/

#define RESP_MODULE(name, setParam, process, init, cleanup) \
    static hex_event::Response HEX_CAT(s_response_,__LINE__)(#name, setParam, process, init, cleanup)

// A response module can request a callback be invoked after specific period of
// inactivity. This function is called if no events were passed to response
// module for processing before 'timeout' seconds elapsed. The timeout function
// takes no arguments and returns no value. Response module can utilize timeout
// functionality for any periodic processing - e.g. flushing events to storage,
// checking connectivity to server etc...
#define RESP_MODULE_TIMTOUT(name, setParam, process, init, cleanup, expire, timeout) \
    static hex_event::Response HEX_CAT(s_response_,__LINE__)(#name, setParam, process, init, cleanup, expire, timeout)

// A response observer can request a callback be invoked for interested settings
#define RESP_OBSERVE(resp, prefix, parse) \
    static hex_event::Observes HEX_CAT(s_observes_, __LINE__)(#resp, #prefix, parse)

/** EVENT_TOPTEN defines interested event categoty for topten list **/

// Declare that an events module will provide a Top Ten List
#define EVENT_TOPTEN(listname) \
    static hex_event::TopTenList HEX_CAT(s_topten_,__LINE__)(#listname)

// Update a Top Ten List with event data
void EventTopTenUpdate(const char* listname, const std::string& key, size_t count, time_t eventTime);

#endif // !__cplusplus

#endif /* endif HEX_EVENT_MODULE_H */
