// HEX SDK

#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/ether.h>

#include <hex/parse.h>

bool
HexParseBool(const char *value, bool *b)
{
    if (!value || *value == '\0')
        return false;

    if (strcasecmp(value, "on") == 0 ||
        strcasecmp(value, "enabled") == 0 ||
        strcasecmp(value, "enable") == 0 ||
        strcasecmp(value, "true") == 0 ||
        strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "1") == 0) {
        if (b)
            *b = true;
        return true;
    }

    if (strcasecmp(value, "off") == 0 ||
        strcasecmp(value, "disabled") == 0 ||
        strcasecmp(value, "disable") == 0 ||
        strcasecmp(value, "false") == 0 ||
        strcasecmp(value, "no") == 0 ||
        strcasecmp(value, "0") == 0) {
        if (b)
            *b = false;
        return true;
    }

    return false;
}

bool
HexParseInt(const char *value, int64_t min, int64_t max, int64_t *n)
{
    if (!value || *value == '\0')
        return false;

    int saved_errno = errno;
    errno = 0;
    char *p = 0;
    // convert string to long integer
    int64_t ll = strtol(value, &p, 0);

    if (errno != 0 || *p != '\0') {
        errno = EINVAL;
        return false;
    }

    if (ll > max || ll < min) {
        errno = ERANGE;
        return false;
    }

    errno = saved_errno;
    if (n)
        *n = ll;
    return true;
}

bool
HexParseUInt(const char *value, uint64_t min, uint64_t max, uint64_t *n)
{
    if (!value || *value == '\0')
        return false;

    int saved_errno = errno;
    errno = 0;
    char *p = 0;
    // convert string to unsigned long integer
    uint64_t ull = strtoul(value, &p, 0);

    if (errno != 0 || *p != '\0') {
        errno = EINVAL;
        return false;
    }

    if (!(ull >= min && ull <= max)) {
        errno = ERANGE;
        return false;
    }

    errno = saved_errno;
    if (n)
        *n = ull;
    return true;

}

bool
HexParseFloat(const char *value, float *f)
{
    if (!value || *value == '\0')
        return false;

    int saved_errno = errno;
    errno = 0;
    char *p = 0;
    float g = strtof(value, &p);

    if (errno != 0 || *p != '\0') {
        errno = EINVAL;
        return false;
    }

    errno = saved_errno;
    if (f)
        *f = g;
    return true;
}

bool
HexParseUIntRange(const char *value, uint64_t min, uint64_t max, uint64_t *from, uint64_t *to)
{
    if (!value || *value == '\0')
        return false;

    uint64_t tmp;
    char *rem = strdup(value);
    char *rid = rem;
    char *first = strsep(&rem, "-");
    if (first) {
        if (!HexParseUInt(first, min, max, &tmp)) {
            free(rid);
            return false;
        }
        *from = tmp;
    }
    if (rem) {
        // Found '-'
        if (!HexParseUInt(rem, min, max, &tmp)) {
            free(rid);
            return false;
        }
        *to = tmp;
    }
    else {
        *to = *from; //FIXME: is this right?
    }
    free(rid);
    return true;
}

bool
HexParseIP(const char *value, int af, void *n)
{
    /*  ip: a.b.c.d |
            x:x:x:x:x:x:x:x |
            ? valid ipv4 ? |
            ? valid ipv6 ?
    */

    if (!value || *value == '\0')
        return false;

    // convert IPv4 and IPv6 addresses from text to binary form
    // returns 1: on success
    //         0: src does not contain a character string
    //        -1: errno is set to EAFNOSUPPORT.
    if (n) {
        if (inet_pton(af, (const char *)value, n) <= 0)
            return false;
    } else {
        unsigned char buf[sizeof(struct in6_addr)];
        if (inet_pton(af, (const char *)value, &buf) <= 0)
            return false;
    }

    return true;
}

bool
HexParseIPRange(const char *value, int af, void* from, void* to)
{
    /*  range:  [!] ip-ip |
                [!] ip/ip |
                [!] ip/digit |
                [!] ip
    */
    if (!value || *value == '\0')
        return false;

    char *second = strdup(value);
    char *rid = second;
    char *first = strsep(&second, "-");
    if (second) {
        // Found '-'
        if (!HexParseIP(first, af, from)) { free(rid); return false; }
        if (!HexParseIP(second, af, to)) { free(rid); return false; }
    } else {
        // No '-', check for '/'
        char *addr = strsep(&first, "/");
        if (!HexParseIP(addr, af, from)) { free(rid); return false; }
        if (first) {
            // Found '/'
            if ((strchr((const char *)first, '.') != NULL) ||
                (strchr((const char *)first, ':') != NULL)) {
                // Parse ip on the right
                if (!HexParseIP(first, af, to)) { free(rid); return false; }
            } else {
                // else digit
                int64_t digit;
                if (af == AF_INET) {
                    if (!HexParseInt(first, 0, 32, &digit)) { free(rid); return false; }
                } else if (af == AF_INET6) {
                    if (!HexParseInt(first, 0, 128, &digit)) { free(rid); return false; }
                }
            }
        } else {
            // single IP
            if (!HexParseIP(addr, af, to)) { free(rid); return false; }
        }
    }
    free(rid);
    return true;
}

bool
HexParseIPList(const char *value, int af)
{
    /*
        list:   range |
                range , list
    */
    char *val = strdup(value);
    char *rid = val;
    char *ipaddr = strsep(&val, ",");
    if (val) {
        // We found an comma, parse the next expression
        // HexParseIPRange , HexParseList
        if (!HexValidateIPRange(ipaddr, af)) { free(rid); return false; }
        if (!HexParseIPList(val, af))  { free(rid); return false; }
    } else {
        // No comma, this is an ipaddress
        if (!HexValidateIPRange(ipaddr, af)) { free(rid); return false; }
    }
    free(rid);
    return true;
}

bool
HexParsePort(const char *value, int64_t *p)
{
    if (!value || *value == '\0')
        return false;

    int64_t port;

    if (!HexParseInt(value, 0, 65535, &port))
        return false;

    if (p)
        *p = port;

    return true;
}

bool
HexParsePortRange(const char *value)
{
    // port: number | number:[number] | [number]:number

    // FIXME: Need to support protocol name perhaps? Slows down iptables processing though
    if (!value || *value == '\0')
        return false;

    char *val = strdup(value);
    char *rid = val;

    if (strchr(val, ':') == NULL) {
        // Just number
        if (!HexParsePort(val, NULL)) { free(rid); return false; }
    }
    else {
        // Found a ':', not just detect numbers
        char *range = strsep(&val, ":");
        bool lr = true, rr = true;

        // ':' in the begin
        if (*range == '\0')
            lr = false;

        // ':' in the last
        if (*val == '\0')
            rr = false;

        // only ':'
        if (!lr && !rr) {
            free(rid);
            return false;
        }

        if (lr)
            if (!HexParsePort(range, NULL)) { free(rid); return false; }
        if (rr)
            if (!HexParsePort(val, NULL)) { free(rid); return false; }
    }

    free(rid);
    return true;
}

bool
HexParseNetmask(const char* mask, int* bits)
{
    struct in_addr subnet = { 0 };
    if (inet_pton(AF_INET, mask, &subnet) == 1) {
        unsigned int m = htonl(subnet.s_addr);
        int i;
        // count 1 number
        for (i = 0; m != 0; m <<= 1, i++);
        *bits = i;
        return true;
    }
    return false;
}

bool
HexParseMACAddress(const char* macAddrStr, unsigned char macAddr[], unsigned int macAddrLen)
{
    struct ether_addr etherAddr;

    // macAddrStr -> NN:NN:NN:NN:NN:NN
    if (!macAddrStr || strlen(macAddrStr) != ETH_ALEN * 2 + 5) {
        return false;
    }

    if (macAddrLen < ETH_ALEN) {
        return false;
    }

    if (!ether_aton_r(macAddrStr, &etherAddr)) {
        return false;
    }

    memcpy(macAddr, &etherAddr.ether_addr_octet, macAddrLen);
    return true;
}

bool
HexParseRegex(const char *value, const char *pattern)
{
    regex_t regex;

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        return false;
    }
    if (regexec(&regex, value, 0, NULL, 0) == REG_NOMATCH) {
        return false;
    } else {
        return true;
    }

    regfree(&regex);
}
