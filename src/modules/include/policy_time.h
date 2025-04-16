// HEX SDK

#ifndef POLICY_TIME_H_
#define POLICY_TIME_H_

#include <hex/yml_util.h>

// The Time configuration type containes date/time and timezone information
struct TimeConfigType
{
    std::string dateTime;
    std::string timeZone;
};

class TimePolicy : public HexPolicy
{
public:
    TimePolicy() : m_initialized(false), m_yml(NULL) {}

    ~TimePolicy()
    {
        if (m_yml)
            FiniYml(m_yml);
    }

    const char* policyName() const { return "time"; }
    const char* policyVersion() const { return "1.0"; }

    bool load(const char* policyFile)
    {
        clear();
        m_initialized = parsePolicy(policyFile);
        return m_initialized;
    }

    bool save(const char* policyFile)
    {
        UpdateYmlValue(m_yml, "timezone", m_cfg.timeZone.c_str());

        return (WriteYml(policyFile, m_yml) == 0);
    }

    // Set the datetime value.
    bool setDateTime(const std::string &dateTime)
    {
        // Return if not initialized
        if (!m_initialized) {
            return false;
        }

        // Set datetime
        m_cfg.dateTime = dateTime;
        return true;
    }

    // Retrieve the datetime value.
    const char* getDateTime()
    {
        return (m_initialized) ? m_cfg.dateTime.c_str() : "";
    }

    // Set the timezone value.
    bool setTimeZone(const std::string &timeZone)
    {
        // Return if not initialized
        if (!m_initialized) {
            return false;
        }

        // Set timezone
        m_cfg.timeZone = timeZone;
        return true;
    }

    // Retrieve the timezone value.
    const char* getTimeZone()
    {
        return (m_initialized) ? m_cfg.timeZone.c_str() : "";
    }

private:
    // Has policy been initialized?
    bool m_initialized;

    // The time level 'config' settings
    TimeConfigType m_cfg;

    // parsed yml N-ary tree
    GNode *m_yml;

    // Clear out any current configuration
    void clear()
    {
        m_initialized = false;

        // Clear the datetime
        m_cfg.dateTime = "";

        // Clear the timezone
        m_cfg.timeZone = "";
    }

    // Method to read the time policy and populate the various member variables
    bool parsePolicy(const char* policyFile)
    {
        if (m_yml) {
            FiniYml(m_yml);
        }
        m_yml = InitYml(policyFile);

        if (ReadYml(policyFile, m_yml) < 0) {
            FiniYml(m_yml);
            return false;
        }

        HexYmlParseString(m_cfg.timeZone, m_yml, "timezone");

        //DumpYmlNode(m_yml);

        return true;
    }
};

#endif /* endif POLICY_TIME_H_ */

