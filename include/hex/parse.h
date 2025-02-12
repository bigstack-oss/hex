// HEX SDK

#ifndef HEX_PARSE_H
#define HEX_PARSE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum ValidateType {
    ValidateNone = 0,
    ValidateIpRange,
};

bool HexParseBool(const char *value, bool *b);

#define HexValidateBool(value) HexParseBool(value, NULL)

// Convert value to an integral type
// If value begins with '0' it is assumed to be represented in octal
// If value begins with '0x' or '0X' it is assumed to be represented in hexadecimal
// Otherwise it is assumed to be represented in decimal
// Returns true if value is a valid integer and within the specified range.
// Otherwise false is returned and errno is set to one of the following:
//
// ERANGE   Given string was out of range.
// EINVAL   Given string did not represent an integral value.
//
// E.g.:
//      int n;
//      int64_t tmp;
//      if (HexParseInt(value, 1, 10, &tmp)) n = tmp;
//
bool HexParseInt(const char *value, int64_t min, int64_t max, int64_t *n);

#define HexValidateInt(value, min, max) HexParseInt(value, min, max, NULL)

bool HexParseFloat(const char *value, float *f);

#define HexValidateFloat(value) HexParseFloat(value, NULL)

// This uses strtoull under the covers so negative values are silently
// converted into their positive representation.
bool HexParseUInt(const char *value, uint64_t min, uint64_t max, uint64_t *n);

#define HexValidateUInt(value, min, max) HexParseUInt(value, min, max, NULL)

bool HexParseUIntRange(const char *value, uint64_t min, uint64_t max, uint64_t *from, uint64_t *to);

#define HexValidateUIntRange(value, min, max) HexParseUInt(value, min, max, NULL, NULL)

// Convert value to IPv4 or IPv6 network address in network byte order
// If address family af is AF_INET, n should be a pointer to a struct in_addr
// If address family af is AF_INET6, n should be a pointer to a struct in6_addr
bool HexParseIP(const char *value, int af, void *n);

// Takes in a range which follows this convention
//  range   := ip-ip | ip/ip | ip/num | ip
//  ip      := ?ipv4 dotted decimal notation? | ?ipv6 notation?
//  num     := ?number between 0 and 64, based on AF?
bool HexParseIPRange(const char *value, int af, void* from, void* to);
#define HexValidateIPRange(value, af) HexParseIPRange(value, af, NULL, NULL)


// Takes in a list of IPRanges
//  list    := range | range ',' list
bool HexParseIPList(const char *value, int af);

bool HexParsePort(const char *value, int64_t *port);
bool HexParsePortRange(const char *value);

// Convert an IPv4 netmask to CIDR-style prefix length
bool HexParseNetmask(const char* mask, int* bits);

// Convert a string MAC address in format NN:NN:NN:NN:NN:NN to byte array
bool HexParseMACAddress(const char* macAddrStr, unsigned char macAddr[], unsigned int macAddrLen);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* ndef HEX_PARSE_H */

