// HEX SDK

#ifndef CLI_TUNINGS_CHANGER_H
#define CLI_TUNINGS_CHANGER_H

#include <hex/cli_util.h>

static const char* LABEL_TUNING_ACTION = "Select action: ";
static const char* LABEL_TUNING_ID = "Select tuning ID: ";
static const char* LABEL_TUNING_ENABLE = "Enable the tuning: ";
static const char* LABEL_TUNING_NAME = "Enter tuning name: ";
static const char* LABEL_TUNING_VALUE = "Enter tuning value: ";

class CliTuningChanger
{
public:
    bool configure(TuningPolicy *policy)
    {
        return construct(policy);
    }

    bool getTuningName(const Tunings &cfg, std::string* name)
    {
        CliPrint(LABEL_TUNING_ID);
        CliList tList;
        std::vector<std::string> namelist;

        int idx = 0;
        namelist.resize(cfg.tunings.size());
        for (auto& t : cfg.tunings) {
            tList.push_back(t.name);
            namelist[idx++] = t.name;
        }

        idx = CliReadListIndex(tList);
        if (idx < 0) {
            return false;
        }

        *name = namelist[idx];
        return true;
    }

private:
    enum {
        ACTION_ADD = 0,
        ACTION_DELETE,
        ACTION_UPDATE,
        ACTION_APPLY,
    };

    bool getAction(int *action)
    {
        CliPrint(LABEL_TUNING_ACTION);
        CliList actList;
        actList.push_back("Add");
        actList.push_back("Delete");
        actList.push_back("Update");
        int idx = CliReadListIndex(actList);
        if (idx < 0) {
            return false;
        }

        *action = idx;
        return true;
    }

    bool setTuningEnable(bool *enabled)
    {
        CliPrint(LABEL_TUNING_ENABLE);
        CliList enList;
        enList.push_back("Enabled");
        enList.push_back("Disabled");
        int idx = CliReadListIndex(enList);
        if (idx < 0 || idx > 1) {
            return false;
        }

        *enabled = (idx == 0) ? true : false;
        return true;
    }

    bool setTuningName(const Tunings &cfg, std::string* name)
    {
        if (!CliReadLine(LABEL_TUNING_NAME, *name)) {
            return false;
        }

        if (!name->length()) {
            CliPrint("Tuning name is required\n");
            return false;
        }

        for (auto& t : cfg.tunings) {
            if (*name == t.name) {
                CliPrint("Duplicate tuning\n");
                return false;
            }
        }

        return true;
    }

    bool setTuningValue(std::string* value)
    {
        if (!CliReadLine(LABEL_TUNING_VALUE, *value)) {
            return false;
        }

        if (!value->length()) {
            CliPrint("Tuning value is required\n");
            return false;
        }

        return true;
    }

    bool construct(TuningPolicy *policy)
    {
        int action = ACTION_ADD;
        if (!getAction(&action)) {
            return false;
        }

        Tunings cfg;
        policy->getTunings(&cfg);
        Tuning t;

        if (action == ACTION_DELETE || action == ACTION_UPDATE) {
            if (!getTuningName(cfg, &t.name)) {
                return false;
            }
        }
        else if (action == ACTION_ADD) {
            if (!setTuningName(cfg, &t.name)) {
                return false;
            }
        }

        switch (action) {
            case ACTION_DELETE:
                if (!policy->delTuning(t.name)) {
                    HexLogError("failed to delete Tuning: %s", t.name.c_str());
                    return false;
                }
                break;
            case ACTION_ADD:
            case ACTION_UPDATE:
                if (!setTuningValue(&t.value)) {
                    return false;
                }
                if (HexSystemF(0, HEX_CFG " validate_tuning_value %s %s", t.name.c_str(), t.value.c_str()) != 0) {
                  HexLogError("failed to validate tuning %s: %s", t.name.c_str(), t.value.c_str());
                  return false;
                }
                if (!setTuningEnable(&t.enabled)) {
                    return false;
                }

                CliPrintf("\n%s tuning: %s = %s (%s)",
                          action == ACTION_ADD ? "Adding" : "Updating",
                          t.name.c_str(), t.value.c_str(),
                          t.enabled ? "Enabled" : "Disabled");

                policy->setTuning(t);
                break;
        }

        return true;
    }
};

#endif /* endif CLI_TUNINGS_CHANGER_H */

