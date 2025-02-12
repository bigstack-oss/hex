// HEX SDK

#include <time.h>

#include <hex/process.h>
#include <hex/log.h>

#include <hex/firsttime_module.h>
#include <hex/firsttime_impl.h>

#include "include/cli_time.h"
#include "include/policy_time.h"

using namespace hex_firsttime;

// All the user visible strings
static const char* LABEL_TIME_TITLE = "Time Configuration";
static const char* LABEL_TIME_DISPLAY = "Time: %s";
static const char* LABEL_TIME_DISPLAY_ERROR = "Error calculating time";
static const char* LABEL_TIME_SET_ERROR = "Error setting time";
static const char* LABEL_DATE_DISPLAY = "Date: %s";
static const char* LABEL_DATE_DISPLAY_ERROR = "Error calculating date";
static const char* LABEL_DATE_SET_ERROR = "Error setting date";
static const char* LABEL_TIMEZONE_DISPLAY = "Time Zone: %s";
static const char* LABEL_TIME_CHANGE_MENU = "Change the time";
static const char* LABEL_DATE_CHANGE_MENU = "Change the date";
static const char* LABEL_TIMEZONE_CHANGE_MENU = "Change the time zone";
static const char* LABEL_TIME_CHANGE_HEADING = "Change the Time";
static const char* LABEL_DATE_CHANGE_HEADING = "Change the Date";
static const char* LABEL_TIMEZONE_CHANGE_HEADING = "Change the Time Zone";
static const char* LABEL_CHANGES_APPLIED_IMMEDIATELY = "Time configuration changes are applied immediately.";
static const char* LABEL_POLICY_TIMEZONE_SYNC_ERROR = "Unable to sync timezone with policy setting '%s'. Set timezone to ensure correct operation.";

static const char* DATETIME_FMT = "\"%Y-%m-%d %H:%M:%S\"";

// The menu module class for time configuration.
class TimeModule : public MenuModule
{
public:
    TimeModule(int order)
     : MenuModule(order, LABEL_TIME_TITLE)
    {
        // Add the timezone change menu item
        addOption(LABEL_TIMEZONE_CHANGE_MENU, LABEL_TIMEZONE_CHANGE_HEADING);

        // Add the date change menu item
        addOption(LABEL_DATE_CHANGE_MENU, LABEL_DATE_CHANGE_HEADING);

        // Add the time change menu item
        addOption(LABEL_TIME_CHANGE_MENU, LABEL_TIME_CHANGE_HEADING);
        m_actionDefaultOnError = STAY;
    }

    // Print a summary of significant menu items
    void summary()
    {
        if (!PolicyManager()->load(m_policy)) {
            CliPrintf("Error retrieving timezone settings");
        }

        printTimeZone();
        printDate();
        printTime();
    }

protected:
    /**
     * Reload the policy each time around the main loop to ensure we're getting
     * committed changes to the policy when a change has not been made by the
     * user.
     *
     * In the case of timezone, also set the timezone based on the policy so
     * that system state is consistent with the display.
     */
    virtual bool loopSetup()
    {
        // Load committed policy
        if (PolicyManager()->load(m_policy, true) == false) {
            return false;
        }

        // Set timezone based on policy setting (see NOTE in doAction())
        std::string tz = m_policy.getTimeZone();
        if (tz == "" || HexSpawn(0, "/usr/sbin/hex_config", "timezone", tz.c_str(), ZEROCHAR_PTR) != 0) {
            CliPrintf(LABEL_POLICY_TIMEZONE_SYNC_ERROR, tz.c_str());
        }

        // Call tzset to recalculate the timezone for up-to-date display
        tzset();
        return true;
    }

    // Print time configuration and immediacy warning as part of the loop header
    virtual void printLoopHeader()
    {
        CliPrintf(LABEL_CHANGES_APPLIED_IMMEDIATELY);
        printTimeZone();
        printDate();
        printTime();
    }

    /**
     *  Perform an action from the list.
     *  The order that was specified in the constructor is:
     */
    virtual bool doAction(int index)
    {
        std::string time, date, timeZone;
        std::string oldTime, newTime;
        std::string oldDate, newDate;

        switch(index)
        {
        case 0:
            // Invoke the timezone builder
            if (m_tzChanger.configure(timeZone)) {
                // Being unable to update the policy is a catastrophic failure
                if (!m_policy.setTimeZone(timeZone)) {
                    return false;
                }

                if (!PolicyManager()->save(m_policy)) {
                    return false;
                }

                // Commit the timezone policy straight away
                if (HexSpawn(0, "/usr/sbin/hex_config", "timezone", timeZone.c_str(), ZEROCHAR_PTR) != 0) {
                    return false;
                }

                // Once localtime has been set, call tzset to recalculate the
                // timezone for up-to-date display
                tzset();
            }
            break;
        case 1:
            getCurrentDate(oldDate);
            if (m_dateChanger.configure(date)) {
                if (!setCurrentDate(date)) {
                    //TODO: HexLogEvent("failed to update date by [admin] via [cli]");
                    return false;
                }
                getCurrentDate(newDate);
                //TODO: HexLogEvent("change date from [oldDate] to [newDate] by [admin] via [cli]");
            }

            break;
        case 2:
            getCurrentTime(oldTime);
            if (m_timeChanger.configure(time)) {
                if (!setCurrentTime(time)) {
                    //TODO: HexLogEvent("failed to update time by [admin] via [cli]");
                    return false;
                }
                getCurrentTime(newTime);
                //TODO: HexLogEvent("change date from [oldTime] to [newTime] by [admin] via [cli]");
            }
            break;
        default:
            return false;
        }
        return true;
    }

private:
    // The various menu item builders
    CliDateChanger m_dateChanger;
    CliTimeChanger m_timeChanger;
    CliTimeZoneChanger m_tzChanger;

    // The time policy to be loaded, modified and saved
    TimePolicy m_policy;

    // Get the current system date adhering to the local timezone
    void getCurrentDate(std::string &date)
    {
        char buf[255];
        struct tm currentTm;
        time_t currentTime;

        // Set the date string to empty for error cases
        date = LABEL_DATE_DISPLAY_ERROR;

        // Get the current time
        if (time(&currentTime) == -1) {
            return;
        }

        // Convert it into a broken time considering localtime
        if (localtime_r(&currentTime, &currentTm) == NULL) {
            return;
        }

        // Format the current time into the CLI presented format
        if (strftime(buf, 255, "%m/%d/%Y", &currentTm) == 0) {
            return;
        }

        // Set the date string to the formatted buffer
        date = buf;
    }

    // Get the current system time adhering to the local timezone
    void getCurrentTime(std::string &timeStr)
    {
        char buf[255];
        struct tm currentTm;
        time_t currentTime;

        // Set the time string to empty for error cases
        timeStr = LABEL_TIME_DISPLAY_ERROR;

        // Get the current time
        if (time(&currentTime) == -1) {
            return;
        }

        // Convert it into a broken time considering localtime
        if (localtime_r(&currentTime, &currentTm) == NULL) {
            return;
        }

        // Format the current time into the CLI presented format
        if (strftime(buf, 255, "%H:%M:%S", &currentTm) == 0) {
            return;
        }

        // Set the time string to the formatted buffer
        timeStr = buf;
    }

    // Set the current date provided a date string
    bool setCurrentDate(std::string &date)
    {
        char buf[255];
        struct tm currentTm, newTm;
        time_t currentTime;

        // Get the current time
        if (time(&currentTime) == -1) {
            return false;
        }

        // Convert it into a broken time considering localtime
        if (localtime_r(&currentTime, &currentTm) == NULL) {
            return false;
        }

        // Convert the formatted input date into a broken time
        // representation
        if (strptime(date.c_str(), "%m/%d/%Y", &newTm) == 0) {
            return false;
        }

        // Set the current date/time with new day, month and year details
        currentTm.tm_mday = newTm.tm_mday;
        currentTm.tm_mon = newTm.tm_mon;
        currentTm.tm_year = newTm.tm_year;

        // Format the updated date/time into a format consumable by hex_config
        if (strftime(buf, 255, DATETIME_FMT, &currentTm) == 0) {
            return false;
        }

        // Perform the system date/time update
        int rc = HexSpawn(0, "/usr/sbin/hex_config", "date", buf, ZEROCHAR_PTR);
        if( rc != 0 )
            CliPrintf(LABEL_DATE_SET_ERROR);

        return (HexExitStatus(rc) == 0);
    }

    // Set the current time provided a time string
    bool setCurrentTime(std::string &timeStr)
    {
        char buf[255];
        struct tm currentTm, newTm;
        time_t currentTime;

        // Get the current time
        if (time(&currentTime) == -1) {
            return false;
        }

        // Convert it into a broken time considering localtime
        if (localtime_r(&currentTime, &currentTm) == NULL) {
            return false;
        }

        // Convert the formatted input time into a broken time
        // representation
        if (strptime(timeStr.c_str(), "%H:%M:%S", &newTm) == 0) {
            return false;
        }

        // Set the current date/time with new hour, minute and second details
        currentTm.tm_hour = newTm.tm_hour;
        currentTm.tm_min = newTm.tm_min;
        currentTm.tm_sec = newTm.tm_sec;

        // Format the updated date/time into a format consumable by hex_config
        if (strftime(buf, 255, DATETIME_FMT, &currentTm) == 0) {
            return false;
        }

        // Perform the system date/time update
        int rc = HexSpawn(0, "/usr/sbin/hex_config", "date", buf, ZEROCHAR_PTR);
        if(rc!=0)
            CliPrintf(LABEL_TIME_SET_ERROR);

        return (HexExitStatus(rc) == 0);
    }

    // Print the current system date
    void printDate()
    {
        std::string date;
        getCurrentDate(date);
        CliPrintf(LABEL_DATE_DISPLAY, date.c_str());
    }

    // Print the current system time
    void printTime()
    {
        std::string time;
        getCurrentTime(time);
        CliPrintf(LABEL_TIME_DISPLAY, time.c_str());
    }

    // Print the current system timezone
    void printTimeZone()
    {
        CliPrintf(LABEL_TIMEZONE_DISPLAY, m_policy.getTimeZone());
    }
};

FIRSTTIME_MODULE(TimeModule, FT_ORDER_LAST + 1);

