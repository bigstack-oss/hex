// HEX SDK

#ifndef HEX_EVENT_ATTR_H
#define HEX_EVENT_ATTR_H

// Required attributes - some effort was made for backwards-comaptibility with ISS products
#define HEX_EVENT_PRIORITY  "priority"         // Use the priority macros above
#define HEX_EVENT_TIMESTAMP "timestamp"        // raw timestamp - unix epoch
#define HEX_EVENT_TIME      "time"             // formatted time
#define HEX_EVENT_ID        "eventid"          // unique identifier (CCCSSNNNNT format)
#define HEX_EVENT_NAME      "name"             // event name; short desc
#define HEX_EVENT_RESP      "responses"        // response list

// allowed constants for HEX_EVENT_PRIORITY attr
#define HEX_EVENT_PRIORITY_HIGH     "high"
#define HEX_EVENT_PRIORITY_MEDIUM   "medium"
#define HEX_EVENT_PRIORITY_LOW      "low"

// System Events
#define HEX_EVENT_DESC      "desc"
#define HEX_EVENT_ARGS      "args"

#endif /* HEX_EVENT_ATTR_H */
