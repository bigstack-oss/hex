// HEX SDK

#ifndef HEX_QUEUE_H
#define HEX_QUEUE_H

#include <sys/uio.h>
#include <mqueue.h>

#ifdef __cplusplus
extern "C" {
#endif

/* one message queue + multiple shm queue design
 * message queue
 *   1. synchronization purpose
 *   2. notify the arrival a message
 *   3. message format uint32_t [source|type]
 *       - source for indicating shm queue
 *       - type for indicating message type
 *
 * shm queue(s)
 *   1. actual message data
 *   2. message format [len|data]
 */

struct Bookkeeping
{
    size_t head;    // head offset
    size_t tail;    // tail offset
};

struct HexQueue
{
    mqd_t mqd;                          // MASTER: message queue file descriptor (synchronization purpose)
    char* name;                         // queue name
    struct Bookkeeping* bookkeeping;    // point to queue header
    char* buf;                          // point to queue data buffer
    size_t size;                        // queue total size (exclude header size)
    int id;                             // queue identifier
};

typedef struct HexQueue *HexQueue_t;

enum {
    HEX_QUEUE_SUCCESS = 0,
    HEX_QUEUE_ERROR = -1,
    HEX_QUEUE_FULL = -2,
    HEX_QUEUE_EMPTY = -3,
    HEX_QUEUE_BUFFER_TOO_SMALL = -4,
};

// Server APIs

#define HexQueueId(q) ((q) ? (q)->id : 0)

int HexQueueInit(const char* mqname, size_t maxmsgs);

HexQueue_t HexQueueAlloc(const char* qname, size_t qsize);

int HexQueueRelease(HexQueue_t q, size_t size);

int HexQueueWait(int mqd, int* qid, int* type, const struct timespec* abs_timeout);

ssize_t HexQueueReceive(HexQueue_t q, char* msg, size_t size);

int HexQueueFini(int mqd);

// Client APIs

HexQueue_t HexQueueAttach(const char* mqname, const char* qname, size_t qsize);

void HexQueueDetach(HexQueue_t q);

int HexQueueSend(HexQueue_t q, const void* ev, size_t len);

int HexQueueSendV(HexQueue_t q, const struct iovec *vec, int count);

int HexQueueSendId(HexQueue_t q, const void* ev, size_t len, int type);

int HexQueueSendVId(HexQueue_t q, const struct iovec *vec, int count, int type);

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_QUEUE_H */
