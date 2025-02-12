// HEX SDK

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/stat.h>
#include <sys/mman.h>

#include <hex/hash.h>
#include <hex/queue.h>

// Align an offset to a multiple of 4
#define align(i) (((i) + sizeof(size_t) - 1) & ~(sizeof(size_t) - 1))

#define min(a,b) ((a) < (b) ? (a) : (b))

#define SHM_PATH "/dev/shm"
#define SHM_PREFIX "/queue_"

// Server APIs
static char*
alloc_name(const char* name)
{
    size_t n = strlen(SHM_PREFIX) + strlen(name) + 1;
    char* s = malloc(n);
    if (s) {
        snprintf(s, n, "%s%s", SHM_PREFIX, name);
    }
    return s;
}

static int
shm_file_exists(const char* name)
{
    int shm_exists = 0;
    size_t n = strlen(SHM_PATH) + strlen(name) + 1;
    char* s = malloc(n);
    if (s) {
        // e.g. /dev/shm/event_system
        snprintf(s, n, "%s%s", SHM_PATH, name);

        struct stat shm_stat;
        shm_exists = (stat(s, &shm_stat) == 0);
        free(s);
    }
    return shm_exists;
}

int
HexQueueInit(const char* mqname, size_t maxmsgs)
{
    struct mq_attr mqa;
    memset(&mqa, 0, sizeof(mqa));
    mqa.mq_maxmsg = maxmsgs;
    mqa.mq_msgsize = sizeof(int);
    return mq_open(mqname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR, &mqa);
}

int
HexQueueFini(int mqd)
{
    return mq_close(mqd);
}

HexQueue_t
HexQueueAlloc(const char* qname, size_t qsize)
{
    if (qname == NULL)
        return NULL;

    HexQueue_t q = (HexQueue_t)malloc(sizeof(struct HexQueue));
    if (!q)
        return NULL;

    q->name = alloc_name(qname);
    q->bookkeeping = 0;
    q->size = 0;
    q->id = HexHash16(qname, strlen(qname));

    int shm_existed = shm_file_exists(q->name);   //check if shm already exists.

    int fd = shm_open(q->name, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        free(q->name);
        q->name = NULL;
        free(q);
        return NULL;
    }

    qsize = align(qsize);

    if (ftruncate(fd, qsize) == -1) {
        free(q->name);
        q->name = NULL;
        free(q);
        close(fd);
        return NULL;
    }

    q->bookkeeping = (struct Bookkeeping*)mmap(NULL, qsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    // Close the file descriptor after the mmap call
    close(fd);

    if (q->bookkeeping == MAP_FAILED) {
        free(q->name);
        q->name = NULL;
        free(q);
        return NULL;
    }

    q->buf = (char*)(q->bookkeeping + 1);
    q->size = qsize - sizeof(struct Bookkeeping);

    // If shm was just created or head or tail exceed usable size, set head and tail
    size_t s = q->size - sizeof(size_t);
    if (!shm_existed ||
        q->bookkeeping->head > s ||
        q->bookkeeping->tail > s) {

        q->bookkeeping->head = 0;
        q->bookkeeping->tail = 0;

        // Initialize head message size to 0 (i.e. empty)
        size_t head = q->bookkeeping->head;
        char* msgPtr = &q->buf[head];
        *((size_t*)msgPtr) = 0;
    }

    return q;
}

int
HexQueueRelease(HexQueue_t q, size_t size)
{
    int rc = 0;
    if (q) {
        rc = munmap(q->bookkeeping, size);
        free(q->name);
        free(q);
    }
    return rc;
}

int
HexQueueWait(int mqd, int* id, int* type, const struct timespec* timeout)
{
    int rc = 0;
    unsigned prio;
    uint32_t message;

    rc = mq_timedreceive(mqd, (char*)&message, sizeof(message), &prio, timeout);
    if (rc == -1) {
        if (errno == ETIMEDOUT || errno == EINTR)
            rc = HEX_QUEUE_EMPTY;
        else
            rc = HEX_QUEUE_ERROR;
    }
    else if (rc > 0)
        rc = 0;

    *id = (message >> 16);
    *type = (message & 0xffff);

    return rc;
}

ssize_t
HexQueueReceive(HexQueue_t q, char* msg, size_t size)
{
    if (!q)
        return -1;

    ssize_t rc = HEX_QUEUE_SUCCESS;
    size_t head = q->bookkeeping->head;
    char* msgPtr = &q->buf[head];
    size_t msgSize = *((size_t*)msgPtr);

    if (msgSize >= q->size) {
        // Invalid message size - something has gone terribly wrong
        q->bookkeeping->head = q->bookkeeping->tail;

        return HEX_QUEUE_ERROR;
    }

    if (msgSize > 0) {
        size_t newHead = align(head + sizeof(size_t) + msgSize) % q->size;

        // Make sure the caller gave us enough room
        if (size >= msgSize) {
            char* readPtr = msgPtr + sizeof(size_t);

            // Check if message wraps
            if (newHead > head || newHead == 0) {
                memcpy(msg, readPtr, msgSize);
            }
            else {
                // Copy first part
                size_t chunk1 = q->size - head - sizeof(size_t);
                memcpy(msg, readPtr, chunk1);
                // Copy the rest
                size_t chunk2 = msgSize - chunk1 ;
                memcpy(msg + chunk1, q->buf, chunk2);
            }

            // Return number of bytes read
            rc = msgSize;

            // Zero out size of message we just read
            *((size_t*)msgPtr) = 0;

            q->bookkeeping->head = newHead;
        }
        else {
            // output buffer too small, notify caller so it might increase buffer and try again.
            rc = HEX_QUEUE_BUFFER_TOO_SMALL;
        }
    }
    else {
        if (head != q->bookkeeping->tail) {
            // Error condition!  Indices show non-empty, but current size field is 0.  Re-sync.
            q->bookkeeping->head = q->bookkeeping->tail;
            rc = HEX_QUEUE_ERROR;
        }
        else
            rc = HEX_QUEUE_EMPTY;
    }

    return rc;
}

// Client APIs

HexQueue_t
HexQueueAttach(const char* mqname, const char* qname, size_t qsize)
{
    if (qname == NULL)
        return NULL;

    HexQueue_t q = (HexQueue_t)malloc(sizeof(struct HexQueue));
    if (!q)
        return NULL;

    q->mqd = mq_open(mqname, O_WRONLY | O_NONBLOCK);
    if (q->mqd == -1) {
        free(q);
        return NULL;
    }

    q->name = alloc_name(qname);
    q->bookkeeping = 0;
    q->size = 0;
    q->id = HexHash16(qname, strlen(qname));

    int fd = shm_open(q->name, O_RDWR, S_IRUSR|S_IWUSR);
    if (fd == -1) {
        free(q->name);
        q->name = NULL;
        free(q);
        return NULL;
    }

    qsize = align(qsize);

    q->bookkeeping = (struct Bookkeeping*)mmap(NULL, qsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    // Close the file descriptor after the mmap call
    close(fd);

    if (q->bookkeeping == MAP_FAILED) {
        free(q->name);
        q->name = NULL;
        free(q);
        return NULL;
    }

    q->buf = (char*)(q->bookkeeping + 1);
    q->size = qsize - sizeof(struct Bookkeeping);

    assert(q->bookkeeping->head <= q->size - sizeof(size_t));
    assert(q->bookkeeping->tail <= q->size - sizeof(size_t));
    assert(q->bookkeeping->head % sizeof(size_t) == 0);
    assert(q->bookkeeping->tail % sizeof(size_t) == 0);

    return q;
}

void
HexQueueDetach(HexQueue_t q)
{
    if (q) {
        mq_close(q->mqd);
        free(q->name);
        free(q);
    }
}

size_t
HexQueueAvail(HexQueue_t q)
{
    size_t space = q->size;
    int head = q->bookkeeping->head;
    int tail = q->bookkeeping->tail;

    if (tail < head)
        space = head - tail;
    else if (head < tail)
        space = q->size - tail + head;

    return space;
}

int
HexQueueSend(HexQueue_t q, const void* ev, size_t len)
{
    if (!q)
        return HEX_QUEUE_ERROR;
    struct iovec vec;
    vec.iov_base = (void*)ev;
    vec.iov_len = len;
    return HexQueueSendVId(q, &vec, 1, q->id);
}

int
HexQueueSendId(HexQueue_t q, const void* ev, size_t len, int type)
{
    if (!q)
        return HEX_QUEUE_ERROR;
    struct iovec vec;
    vec.iov_base = (void*)ev;
    vec.iov_len = len;
    return HexQueueSendVId(q, &vec, 1, type);
}

int
HexQueueSendV(HexQueue_t q, const struct iovec *vec, int count)
{
    if (!q)
        return HEX_QUEUE_ERROR;
    return HexQueueSendVId(q, vec, count, q->id);
}

int
HexQueueSendVId(HexQueue_t q, const struct iovec *vec, int count, int type)
{
    if (!q)
        return HEX_QUEUE_ERROR;

    int rc = HEX_QUEUE_SUCCESS;

    size_t size = 0;
    int vecIdx;
    for (vecIdx = 0; vecIdx < count; ++vecIdx)
         size += vec[vecIdx].iov_len;
    size = align(size);

    int tail = q->bookkeeping->tail;

    // Check if there's enough space between tail and head
    size_t space = HexQueueAvail(q);
    if (space <= size + sizeof(size_t)) // need to account for overhead (add header)
        return HEX_QUEUE_FULL; // message won't fit

    // msg format: [size|msg]
    char* msgPtr = &q->buf[tail];
    size_t dest = align(tail + sizeof(size_t)) % q->size;   // add header
    char* destPtr = &q->buf[dest];
    int newTail = align(dest + size);

    if (newTail <= q->size) {   // likely
        // Non-wrapping case

        for (vecIdx = 0; vecIdx < count; ++vecIdx) {
            memcpy(destPtr, vec[vecIdx].iov_base, vec[vecIdx].iov_len);
            destPtr += vec[vecIdx].iov_len;
        }
    }
    else {
        // Handle wrap-around

        size_t chunk[2];
        chunk[0] = q->size - dest;
        chunk[1] = size - chunk[0];
        size_t len = size;
        int i = 0;
        int j = 0;
        size_t u = 0;
        size_t v = 0;

        while (len) {
            assert(j < 2);
            size_t n = min(chunk[j] - v, vec[i].iov_len - u);
            memcpy(destPtr, (char*)vec[i].iov_base + u, n);
            destPtr += n;
            u += n;
            v += n;
            if (u == vec[i].iov_len) {
                if (++i == count)
                    break;
                u = 0;
            }

            if (v == chunk[j]) {
                destPtr = q->buf; // reset to beginning of ringbuffer
                j++;
                v = 0;
            }
            len -= n;

            if (j > 1) // Something bad happened; should be done already
                return HEX_QUEUE_ERROR;
        }
    }
    newTail %= q->size;

    // Zero out size field of next message
    *((size_t*)&q->buf[newTail]) = 0;

    // MUST write message data first, then size!!!
    *((size_t*)msgPtr) = size;

    // Send notification to server side
    uint32_t message = (q->id << 16) | (type);
    rc = mq_send(q->mqd, (char*)&message, sizeof(message), 0); //TODO: set prio?
    if (rc == -1) {
        if (errno == EAGAIN) {
            // already committed msg to shm queue; so un-commit
            *((size_t*)msgPtr) = 0;
            rc = HEX_QUEUE_FULL;
        }
        else //TODO: special case for EINTR?
            rc = HEX_QUEUE_ERROR;
    }
    else {
        // Update writePtr
        q->bookkeeping->tail = newTail;
    }

    assert(q->bookkeeping->head <= q->size - sizeof(size_t));
    assert(q->bookkeeping->tail <= q->size - sizeof(size_t));
    assert(q->bookkeeping->head % sizeof(size_t) == 0);
    assert(q->bookkeeping->tail % sizeof(size_t) == 0);

    return rc;
}

