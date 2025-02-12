// HEX SDK

#ifndef HEX_LICENSE_H
#define HEX_LICENSE_H

#define EXEMPTION_PERIOD    604800  /* 7 days  */
#define ONBOOT_TIMER        "72hours"
#define CHECK_TIMER         "24hours"

#define LICENSE_OK    0

#define LICENSE_BADSYS      -1
#define LICENSE_BADSIG      -2
#define LICENSE_BADHW       -3
#define LICENSE_NOEXIST     -4
#define LICENSE_EXPIRED     -5
#define CHECKER_RECREATE    -8
#define TIMER_RECREATE      -9
#define RECREATE_FAILED     -10

// Process extended API requires C++
#ifdef __cplusplus

#include <string>

int HexLicenseCheck(const std::string& app, std::string *type, std::string *serial, const std::string& filename = "");

#endif // __cplusplus

#endif /* endif HEX_LICENSE_H */

