// HEX SDK

#ifndef POLICY_TUNING_H
#define POLICY_TUNING_H

#include <list>

#include <hex/yml_util.h>
#include <hex/cli_util.h>

struct Tuning
{
    bool enabled;
    std::string name;
    std::string value;
    Tuning() : enabled(true), name(""), value("") {}
};

struct Tunings
{
    bool enabled;
    std::list<Tuning> tunings;
};

class TuningPolicy : public HexPolicy
{
public:
    TuningPolicy() : m_initialized(false), m_yml(NULL) {}

    ~TuningPolicy()
    {
        if (m_yml) {
            FiniYml(m_yml);
            m_yml = NULL;
        }
    }

    const char* policyName() const { return "tuning"; }
    const char* policyVersion() const { return "1.0"; }

    bool load(const char* policyFile)
    {
        clear();
        m_initialized = parsePolicy(policyFile);
        return m_initialized;
    }

    bool save(const char* policyFile)
    {
        UpdateYmlValue(m_yml, "enabled", m_cfg.enabled ? "true" : "false");

        DeleteYmlNode(m_yml, "tunings");
        if (m_cfg.tunings.size()) {
            AddYmlKey(m_yml, NULL, "tunings");
        }

        // seq index starts with 1
        int idx = 1;
        for (auto& t : m_cfg.tunings) {
            char seqIdx[64], tuningPath[256];

            snprintf(seqIdx, sizeof(seqIdx), "%d", idx);
            snprintf(tuningPath, sizeof(tuningPath), "tunings.%d", idx);

            AddYmlKey(m_yml, "tunings", (const char*)seqIdx);
            AddYmlNode(m_yml, tuningPath, "enabled", t.enabled ? "true" : "false");
            AddYmlNode(m_yml, tuningPath, "name", t.name.c_str());
            AddYmlNode(m_yml, tuningPath, "value", t.value.c_str());

            idx++;
        }

        return (WriteYml(policyFile, m_yml) == 0);
    }

    void updateTuning(Tuning *dst, const Tuning &src)
    {
        dst->enabled = src.enabled;
        dst->name = src.name;
        dst->value = src.value;
    }

    bool setTuning(const Tuning &tuning)
    {
        if (!m_initialized) {
            return false;
        }

        // update existing tuning
        for (auto& t : m_cfg.tunings) {
            if (tuning.name == t.name) {
                updateTuning(&t, tuning);
                return true;
            }
        }

        // add new tuning
        Tuning newTuning;
        newTuning.name = tuning.name;
        updateTuning(&newTuning, tuning);

        m_cfg.tunings.push_back(newTuning);

        return true;
    }

    Tuning* getTuning(const std::string& name, int* tid)
    {
        *tid = 0;

        if (!m_initialized) {
            return NULL;
        }

        int id = 1;
        for (auto& t : m_cfg.tunings) {
            if (name == t.name) {
                Tuning *ptr = &t;
                *tid = id;
                return ptr;
            }
            id++;
        }

        return NULL;
    }

    bool delTuning(const std::string &name)
    {
        if (!m_initialized) {
            return false;
        }

        for (auto it = m_cfg.tunings.begin(); it != m_cfg.tunings.end(); ++it) {
            if (name == it->name) {
                m_cfg.tunings.erase(it);
                return true;
            }
        }

        return true;
    }

    bool getTunings(Tunings *config) const
    {
        if (!m_initialized) {
            return false;
        }

        config->enabled = m_cfg.enabled;
        config->tunings = m_cfg.tunings;

        return true;
    }

private:
    // Has policy been initialized?
    bool m_initialized;

    // The time level 'config' settings
    Tunings m_cfg;

    // parsed yml N-ary tree
    GNode *m_yml;

    // Clear out any current configuration
    void clear()
    {
        m_initialized = false;
        m_cfg.enabled = true;
        m_cfg.tunings.clear();
    }

    // Method to read the role policy and populate the role member variables
    bool parsePolicy(const char* policyFile)
    {
        if (m_yml) {
            FiniYml(m_yml);
            m_yml = NULL;
        }
        m_yml = InitYml(policyFile);

        if (ReadYml(policyFile, m_yml) < 0) {
            FiniYml(m_yml);
            m_yml = NULL;
            return false;
        }

        HexYmlParseBool(&m_cfg.enabled, m_yml, "enabled");

        size_t num = SizeOfYmlSeq(m_yml, "tunings");
        for (size_t i = 1 ; i <= num ; i++) {
            Tuning obj;

            HexYmlParseBool(&obj.enabled, m_yml, "tunings.%d.enabled", i);
            HexYmlParseString(obj.name, m_yml, "tunings.%d.name", i);
            HexYmlParseString(obj.value, m_yml, "tunings.%d.value", i);

            m_cfg.tunings.push_back(obj);
        }

        //DumpYmlNode(m_yml);

        return true;
    }
};

#endif /* endif POLICY_TUNING_H */

