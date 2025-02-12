// HEX SDK

#ifndef SETTING_NETWORK_H
#define SETTING_NETWORK_H

#include <arpa/inet.h>

#include <vector>
#include <map>
#include <algorithm>

#include <regex.h>

#include <hex/log.h>
#include <hex/tuning.h>
#include <hex/process.h>
#include <hex/process_util.h>

#include "include/policy_network.h"

typedef std::vector<std::string> StringList;
typedef std::map<std::string, std::string> StringMap;

/**
 * Representation of the interface system settings
 * (read from setting.sys)
 */
class InterfaceSystemSettings
{
public:
    InterfaceSystemSettings() {}

    // build a interface settings
    void build(const char* name, const char* value)
    {
        const char* p = 0;
        if (HexMatchPrefix(name, "sys.net.if.label.", &p)) {
            m_port2Label[p] = value;
            m_interfaces.push_back(p);
            std::sort(m_interfaces.begin(), m_interfaces.end(), [](const std::string &s1, const std::string &s2) {
                std::string v1 = HexUtilPOpen("echo %s | sed 's/[^0-9]*//g'", s1.c_str());
                std::string v2 = HexUtilPOpen("echo %s | sed 's/[^0-9]*//g'", s2.c_str());
                return std::stoi(v1) < std::stoi(v2);
            });
        }
    }

    bool port2Label(const std::string &port, std::string *label) const
    {
        StringMap::const_iterator labelIter = m_port2Label.find(port);
        if (labelIter != m_port2Label.end()) {
            *label = labelIter->second;
            return true;
        }

        return false;
    }

    bool label2Port(const std::string &label, std::string *port) const
    {
        for (StringList::const_iterator netIter = m_interfaces.begin();
             netIter != m_interfaces.end(); ++netIter) {
            StringMap::const_iterator labelIter = m_port2Label.find(*netIter);
            if (labelIter != m_port2Label.end() && label == labelIter->second) {
                *port = labelIter->first;
                return true;
            }
        }
        return false;
    }

    // A begin() iterator over all the network interface ports
    StringList::const_iterator netIfBegin() const { return m_interfaces.begin(); }

    // The end() iterator over all the network interfaces ports
    StringList::const_iterator netIfEnd()   const { return m_interfaces.end(); }

    // The number of network interfaces
    size_t netIfSize()                      const { return m_interfaces.size(); }

private:

    // The list of network interfaces (ex. [eth0, eth1, ...])
    StringList m_interfaces;

    // A mapping from interface to label. (ex. eth0 => IF.1)
    StringMap  m_port2Label;

};

/**
 * Representation of the device system settings
 * (read from 'ip' and 'ethtool')
 */
class DeviceSystemSettings
{
public:
    DeviceSystemSettings()
     : m_initialized(false)
    {
        m_initialized = initialize();
    }

    ~DeviceSystemSettings()
    {
        if (m_initialized) {
            regfree(&m_inet);
            regfree(&m_inet6);
            regfree(&m_defaultGw);
            regfree(&m_linkRe);
            regfree(&m_autonegRe);
            regfree(&m_speedRe);
        }
    }

    // The build method constructs a device settings object for a given interface.
    // If it cannot successfully construct such an object, it returns NULL
    bool getDevicePolicy(DevicePolicy &policy, const std::string &ifName)
    {
        if (!m_initialized) {
            HexLogError("DeviceSystemSettings: not initialized");
            return false;
        }

        return construct(policy, ifName);
    }

private:
    // Has this object successfully completed initialization
    bool m_initialized;

    // Regexes that match the output of 'ip address show' for IPv4 and IPv6 lines
    regex_t m_inet;
    regex_t m_inet6;


    // Regex that matches the output of 'ip route' for the default gateway. This
    // works for both IPv4 and IPv6
    regex_t m_defaultGw;

    // Regexs for speed, link, autoneg, and duplex
    regex_t m_linkRe;
    regex_t m_autonegRe;
    regex_t m_speedRe;
    regex_t m_duplexRe;

    bool initialize()
    {
        // Compile the regular expressions
        if (regcomp(&m_inet, "^\\s+inet\\s+([^/]+)/([[:digit:]]+).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of IPv4 regex failed.");
            return false;
        }

        if (regcomp(&m_inet6, "^\\s+inet6\\s+([^/]+)/([^[:space:]]+).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of IPv6 regex failed.");
            regfree(&m_inet);
            return false;
        }

        if (regcomp(&m_defaultGw, "^default via ([^[:space:]]+) dev ([^[:space:]]+).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of default gatway regex failed.");
            regfree(&m_inet);
            regfree(&m_inet6);
            return false;
        }

        if (regcomp(&m_linkRe, "\\s+Link detected: ([a-z]*).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of Link detection regex failed.");
            regfree(&m_inet);
            regfree(&m_inet6);
            regfree(&m_defaultGw);
            return false;
        }
        if (regcomp(&m_autonegRe, "\\s+Auto-negotiation: ([a-z]*).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of Autoneg regex failed.");
            regfree(&m_inet);
            regfree(&m_inet6);
            regfree(&m_defaultGw);
            regfree(&m_linkRe);
            return false;
        }

        if (regcomp(&m_speedRe, "\\s+Speed: ([0-9]*).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of speed regex failed.");
            regfree(&m_inet);
            regfree(&m_inet6);
            regfree(&m_defaultGw);
            regfree(&m_linkRe);
            regfree(&m_autonegRe);
            return false;
        }
        if (regcomp(&m_duplexRe, "\\s+Duplex: ([a-zA-Z]*).*$",
                    REG_EXTENDED | REG_NEWLINE) != 0) {
            HexLogError("Compilation of Duplex regex failed.");
            regfree(&m_inet);
            regfree(&m_inet6);
            regfree(&m_defaultGw);
            regfree(&m_linkRe);
            regfree(&m_autonegRe);
            regfree(&m_speedRe);
            return false;
        }

        return true;
    }

    // Set the value of target based on the regular expression match of output
    inline void setValue(std::string &target, std::string &output, regmatch_t &match)
    {
        target = output.substr(match.rm_so, match.rm_eo - match.rm_so);
    }

    // Convert a prefix into a netmask and store it in the target
    void convertPrefixToNetmask(std::string &target, const std::string &prefixStr)
    {
        target = "";
        int prefix = atoi(prefixStr.c_str());

        if (prefix >= 0 && prefix <= 32) {
            uint32_t val = 0;
            uint32_t currBit = 1 << 31;

            // calc netmask value based on prefix
            for (int idx = 0; idx < prefix; ++idx) {
                val |= currBit;
                currBit >>= 1;
            }

            struct in_addr subnet;
            subnet.s_addr = htonl(val);
            char buffer[INET_ADDRSTRLEN];

            memset((void *)buffer, 0, sizeof(buffer));
            if (inet_ntop(AF_INET, &subnet, buffer, sizeof(buffer)) != 0) {
                target = buffer;
            }
        }
    }

    // Get the default gateway and store it in the parameter provided
    bool getDefaultGateway(bool ipv4, std::string &gateway, const std::string &ifName)
    {
        char cmd[256];

        if (ipv4) {
            snprintf(cmd, sizeof(cmd), "/usr/sbin/ip -f inet route");
        }
        else {
            snprintf(cmd, sizeof(cmd), "/usr/sbin/ip -f inet6 route");
        }
        std::string output;
        int exitStatus = -1;

        if (!(HexRunCommand(exitStatus, output, cmd) && exitStatus == 0)) {
            return false;
        }

        regmatch_t match[3];
        size_t nmatch = sizeof(match) / sizeof(match[0]);

        // Match 1 is the gateway address, 2 is the interface
        if (regexec(&m_defaultGw, output.c_str(), nmatch, match, 0) == 0) {
            std::string defaultGwIf;
            setValue(defaultGwIf, output, match[2]);
            if (defaultGwIf == ifName) {
                setValue(gateway, output, match[1]);
            }
        }

        return true;
    }


    // Perform the actual initialization of the settings object. Returns true if
    // all settings could be successfully retrieved, false otherwise
    bool construct(DevicePolicy &policy, const std::string &ifName)
    {
        std::string output = "";
        int exitStatus = -1;

        // First run ip address show to get the IPv4 / IPv6 addresses and masks
        // Only try and parse the output if the command ran and exited with code 0
        if (!(HexRunCommand(exitStatus, output, "/usr/sbin/ip address show %s", ifName.c_str()) &&
             exitStatus == 0)) {
            HexLogError("DeviceSystemSettings: ip address show failed");
            return false;
        }

        // Check for status of UP in the first line
        std::vector<std::string> lines = hex_string_util::split(output, '\n');
        if (lines[0].find("UP") != std::string::npos) {
            HexLogDebug("Interface: %s, enabled: true", ifName.c_str());
            policy.m_enabled = true;
        } else {
            HexLogDebug("Interface: %s, enabled: false", ifName.c_str());
            policy.m_enabled = false;
        }

        // If the interface is enabled, find its IPv4 and IPv6 addresses
        if (policy.m_enabled) {
            regmatch_t match[3];
            size_t nmatch = sizeof(match) / sizeof(match[0]);

            // Match 1 is the IPv4 Address, 2 is the prefix
            if (regexec(&m_inet, output.c_str(), nmatch, match, 0) == 0) {
                policy.m_ipv4.m_mode = AddressPolicy::AVAILABLE;
                setValue(policy.m_ipv4.m_address, output, match[1]);
                // Convert the prefix value to a netmask.
                std::string prefix;
                setValue(prefix, output, match[2]);
                convertPrefixToNetmask(policy.m_ipv4.m_mask, prefix);
            }

            // Match 1 is the IPv6 address, 2 is the prefix
            if (regexec(&m_inet6, output.c_str(), nmatch, match, 0) == 0) {
                policy.m_ipv6.m_mode = AddressPolicy::AVAILABLE;
                setValue(policy.m_ipv6.m_address, output, match[1]);
                setValue(policy.m_ipv6.m_mask, output, match[2]);
            }
        }

        if (policy.m_enabled &&
            policy.m_ipv4.m_mode == AddressPolicy::AVAILABLE) {
            getDefaultGateway(true, policy.m_ipv4.m_gateway, ifName);
        }

        if (policy.m_enabled &&
            policy.m_ipv6.m_mode == AddressPolicy::AVAILABLE) {
            getDefaultGateway(false, policy.m_ipv6.m_gateway, ifName);
        }

        // Run ethtool to grab the Link state, autoneg, speed, and duplex
        if (policy.m_enabled) {
            // First run ip address show to get the IPv4 / IPv6 addresses and masks
            output = "";
            exitStatus = -1;

            // Only try and parse the output if the command ran and exited with code 0
            if (!(HexRunCommand(exitStatus, output, "/sbin/ethtool %s", ifName.c_str()) && exitStatus == 0)) {
                HexLogError("DeviceSystemSettings: ethtool failed");
                return false;
            }

            // Check for status of UP in the first line
            StringList lines = hex_string_util::split(output, '\n');

            regmatch_t match[3];
            size_t nmatch = sizeof(match) / sizeof(match[0]);

            // link detected
            if (regexec(&m_linkRe, output.c_str(), nmatch, match, 0) == 0) {
                setValue(policy.m_gotLink, output, match[1]);
            }
            // Auto-neg
            if (regexec(&m_autonegRe, output.c_str(), nmatch, match, 0) == 0) {
                setValue(policy.m_autoneg, output, match[1]);
            }
            // Speed
            if (regexec(&m_speedRe, output.c_str(), nmatch, match, 0) == 0) {
                setValue(policy.m_speed, output, match[1]);
            }
            // Duplex
            if (regexec(&m_duplexRe, output.c_str(), nmatch, match, 0) == 0) {
                setValue(policy.m_duplex, output, match[1]);
            }
        }

        policy.m_initialized = true;

        return true;
    }

};

#endif /* endif SETTING_NETWORK_H */

