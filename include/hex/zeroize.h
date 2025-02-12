// HEX SDK

#ifndef HEX_ZEROIZE_H
#define HEX_ZEROIZE_H

#include <unistd.h>

#include <strings.h>

#include <hex/log.h>
#include <hex/process.h>

// delete a file securely, first overwriting it to hide its contents
#define SHRED_UTIL "/usr/bin/shred"

#ifdef __cplusplus

#include <string>

static inline
void HexZeroizeMemory(std::string& s)
{
    //iterate over the string and set each char to "zero"
    for (std::string::iterator it = s.begin(); it != s.end(); ++it) {
        *it = '\0';
    }
}

static inline
void HexZeroizeMemory(std::wstring& s)
{
     //iterate over the string and set each char to "zero"
    for (std::wstring::iterator it = s.begin(); it != s.end(); ++it) {
         *it = '\0';
     }

}

extern "C" {
#endif

// header to be used in cases were a function needs to zeroize a file.
// Use this function in places where transitory information should be destroyed to meet
// requirements to zerioze Critical Security Parameters such as crytographic artifacts or plain
// text sensative data.
// Note this only writes random data, followed by zeros. It does not delete the file.
static inline
int HexZeroizeFile(const char* filename)
{
    HexLogDebugN(2, "Executing %s against file %s", SHRED_UTIL, filename);

    int status = 0;
    if (access(filename, F_OK) == 0) {
        if (HexSpawn(0, SHRED_UTIL, "-z", filename, NULL) != 0) {
            HexLogWarning("Zeroization failed for file %s.", filename);
            status = 1;
        }
        else {
            HexLogInfo("Successfully zeroized file %s.", filename);
            status = 0;
        }
    }
    else {
        HexLogWarning("Zeroization failed: File %s does not exist.", filename);
        status = 1;
    }

    return status;
}

static inline
void HexZeroizeMemory(void *s, size_t n)
{
    if (s == NULL) {
        HexLogWarning("Pointer supplied is NULL, skipping zeroise memory");

    }
    else {
        HexLogDebugN(2, "Setting memory length %lu at address %p to zero", n, s);
        bzero(s,n);
    }

}

#ifdef __cplusplus
}
#endif

#endif /* endif HEX_ZEROIZE_H */

