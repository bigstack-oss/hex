// HEX SDK

#ifndef CLI_TIME_H
#define CLI_TIME_H

#include <hex/string_util.h>

// All the user-visible strings
static const char* LABEL_CONFIG_DATE = "Enter the Date [MM/DD/YYYY]:";
static const char* LABEL_CONFIG_TIME = "Enter the Time [HH:MM:SS]:";
static const char* LABEL_CONFIG_CONTINENT = "Select a continent or ocean:";
static const char* LABEL_CONFIG_TIMEZONE = "Select a timezone:";
static const char* MSG_INVALID_DATE = "The value entered, %s, is not a valid date of the format MM/DD/YYYY or the year is earlier than 1970.";
static const char* MSG_INVALID_TIME = "The value entered, %s, is not a valid time of the format HH:MM:SS.";

// Define a timezone item type to associate continents with a list of timezones
typedef std::pair< std::string, std::vector<std::string> > timeZoneItem;

// A class to build and validate date for policy based on user provided input.
class CliDateChanger {
public:
    CliDateChanger() {}

    // configure date based on user selection via CLI
    bool configure(std::string &date)
    {
        if (!CliReadLine(LABEL_CONFIG_DATE, date)) {
            return false;
        }
        return validate(date);
    }

    // Validate user time input
    bool validate(const std::string &date)
    {
        bool result = true;

        // Number of days per month starting at zero
        int daysPerMonth[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        // The date format is fixed and must be 10 characters in length
        if (date.length() != 10) {
            result = false;
        }
        else {
            struct tm tm;

            // The format is composed of month, day and year separated by '/' characters
            if (!strptime(date.c_str(), "%m/%d/%Y", &tm)) {
                result = false;
            }
            else {
                // Check year, month, day and leap year
                int year = tm.tm_year + 1900;
                int month = tm.tm_mon + 1;
                int day = tm.tm_mday;
                if ((year < 1970 || year > 9999) ||
                    (month < 1 || month > 12) ||
                    (day < 1 || day > daysPerMonth[month - 1]) ||
                    (month == 2 && day == 29 &&
                     !(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))) {
                    result = false;
                }
            }
        }

        // The date string was not valid, return an error message
        if (!result) {
            CliPrintf(MSG_INVALID_DATE, date.c_str());
            //TODO: HexLogEvent("Invalid date input by [Admin] via [CLI]");
        }
        return result;
    }
};

// A class to build and validate time for policy based on user provided input.
class CliTimeChanger
{
public:
    CliTimeChanger() {}

    // configure time based on user selection via CLI
    bool configure(std::string &time)
    {
        if (!CliReadLine(LABEL_CONFIG_TIME, time))
        {
            return false;
        }
        return validate(time);
    }

    // Validate user time input
    bool validate(const std::string &time)
    {
        bool result = true;
        struct tm tm;

        // The time format is fixed and must be 8 characters in length,
        // must be able to be parsed correctly by strptime,
        // and must adhere to the strict sizing implied by HH:MM:SS
        if (time.length() != 8 || !strptime(time.c_str(), "%H:%M:%S", &tm)) {
            result = false;
        }
        else {
            // Split the time on ':' and check sizing and range
            std::vector<std::string> ftime = hex_string_util::split(time, ':');
            assert(ftime.size() == 3);

            // The seconds value entry can be permissive based on leap seconds
            // and valid values as accepted by date and strptime(). A
            // hard check is required to ensure time validity.
            std::stringstream secString(ftime[2]);
            unsigned sec;
            secString >> sec;
            if (ftime[0].size() != 2 || ftime[1].size() != 2 ||
                ftime[2].size() != 2 || sec > 60) {
                result = false;
            }
        }

        // The date string was not valid, return an error message
        if (!result) {
            CliPrintf(MSG_INVALID_TIME, time.c_str());
            //TODO: HexLogEvent("Invalide time configured by [admin] via [CLI]");
        }
        return result;
    }
};


/**
 * A class to build and validate time for policy based on user provided input.
 */
class CliTimeZoneChanger
{
public:
    CliTimeZoneChanger ()
    {
        createValidTimeZones();
    }

    // configure timezone based on user selection via CLI
    bool configure(std::string &timezone)
    {
        // Query for continent
        CliPrintf(LABEL_CONFIG_CONTINENT);

        CliList continentList;
        for (std::vector<timeZoneItem>::const_iterator iter = m_validTimeZones.begin();
             iter != m_validTimeZones.end(); ++iter) {
            continentList.push_back(iter->first);
        }

        int idx = CliReadListIndex(continentList);
        if (idx < 0 || (unsigned int) idx >= m_validTimeZones.size()) {
            return false;
        }

        // Query for timezone
        CliPrintf(LABEL_CONFIG_TIMEZONE);

        CliList tzList;
        timeZoneItem tzi = m_validTimeZones[idx];
        for (std::vector<std::string>::const_iterator iter = tzi.second.begin();
             iter != tzi.second.end(); ++iter) {
            tzList.push_back(*iter);
        }

        idx = CliReadListIndex(tzList);
        if (idx < 0 || (unsigned int)idx >= tzi.second.size()) {
            return false;
        }

        // The timezone has been selected; remove the ease-of-use
        // UTC offset and set. Note that there is only one space in any of
        // the recognized timezones.
        std::vector<std::string> tz = hex_string_util::split(tzi.second[idx], ' ');
        assert(tz.size() == 2);
        timezone = tz[1];

        return true;
    }

private:
    // Valid time zones used by the builder
    std::vector<timeZoneItem> m_validTimeZones;

    // Generate a vector of all timeZoneItems for all recognized timezones.
    void createValidTimeZones(void)
    {
/*
// run the below RoR code at Cloud9 (RoR project)
// for generating timezones list

require "active_support"
require "active_support/time_with_zone"
zones = ActiveSupport::TimeZone::MAPPING.values.uniq

puts ("\nTimezone for CLI")
zones.each do |i|
offset = "(UTC" + ActiveSupport::TimeZone.create(i).formatted_offset + ")"
puts ("\"%s %s\"," % [offset, i])
puts ("\"" + offset + " " + i + "\",")
end

puts ("\nTimezone for LMI")
puts ("export const timezones = [")
zones.each do |i|
offset = "(UTC" + ActiveSupport::TimeZone.create(i).formatted_offset + ")"
puts ("  { name: '%s', offset: '%s' }," % [i, offset])
end
puts ("];")

puts ("\nTotal %d timezones" % zones.length)
*/
        // Define the recognized timezones by continent to add to the valid vector
        const char *africaZones[]   = { "(UTC+00:00) Africa/Casablanca",
                                        "(UTC+00:00) Africa/Monrovia",
                                        "(UTC+01:00) Africa/Algiers",
                                        "(UTC+02:00) Africa/Cairo",
                                        "(UTC+02:00) Africa/Harare",
                                        "(UTC+02:00) Africa/Johannesburg",
                                        "(UTC+03:00) Africa/Nairobi" };

        const char *americaZones[]  = { "(UTC-09:00) America/Juneau",
                                        "(UTC-08:00) America/Los_Angeles",
                                        "(UTC-08:00) America/Tijuana",
                                        "(UTC-07:00) America/Denver",
                                        "(UTC-07:00) America/Phoenix",
                                        "(UTC-07:00) America/Chihuahua",
                                        "(UTC-07:00) America/Mazatlan",
                                        "(UTC-06:00) America/Chicago",
                                        "(UTC-06:00) America/Regina",
                                        "(UTC-06:00) America/Mexico_City",
                                        "(UTC-06:00) America/Monterrey",
                                        "(UTC-06:00) America/Guatemala",
                                        "(UTC-05:00) America/New_York",
                                        "(UTC-05:00) America/Indiana/Indianapolis",
                                        "(UTC-05:00) America/Bogota",
                                        "(UTC-05:00) America/Lima",
                                        "(UTC-04:00) America/Halifax",
                                        "(UTC-04:00) America/Caracas",
                                        "(UTC-04:00) America/La_Paz",
                                        "(UTC-04:00) America/Santiago",
                                        "(UTC-03:30) America/St_Johns",
                                        "(UTC-03:00) America/Sao_Paulo",
                                        "(UTC-03:00) America/Argentina/Buenos_Aires",
                                        "(UTC-03:00) America/Montevideo",
                                        "(UTC-04:00) America/Guyana",
                                        "(UTC-03:00) America/Godthab" };

        const char *asiaZones[]     = { "(UTC+02:00) Asia/Jerusalem",
                                        "(UTC+03:00) Asia/Kuwait",
                                        "(UTC+03:00) Asia/Riyadh",
                                        "(UTC+03:00) Asia/Baghdad",
                                        "(UTC+03:30) Asia/Tehran",
                                        "(UTC+04:00) Asia/Muscat",
                                        "(UTC+04:00) Asia/Baku",
                                        "(UTC+04:00) Asia/Tbilisi",
                                        "(UTC+04:00) Asia/Yerevan",
                                        "(UTC+04:30) Asia/Kabul",
                                        "(UTC+05:00) Asia/Yekaterinburg",
                                        "(UTC+05:00) Asia/Karachi",
                                        "(UTC+05:00) Asia/Tashkent",
                                        "(UTC+05:30) Asia/Kolkata",
                                        "(UTC+05:45) Asia/Kathmandu",
                                        "(UTC+06:00) Asia/Dhaka",
                                        "(UTC+05:30) Asia/Colombo",
                                        "(UTC+06:00) Asia/Almaty",
                                        "(UTC+07:00) Asia/Novosibirsk",
                                        "(UTC+06:30) Asia/Rangoon",
                                        "(UTC+07:00) Asia/Bangkok",
                                        "(UTC+07:00) Asia/Jakarta",
                                        "(UTC+07:00) Asia/Krasnoyarsk",
                                        "(UTC+08:00) Asia/Shanghai",
                                        "(UTC+08:00) Asia/Chongqing",
                                        "(UTC+08:00) Asia/Hong_Kong",
                                        "(UTC+06:00) Asia/Urumqi",
                                        "(UTC+08:00) Asia/Kuala_Lumpur",
                                        "(UTC+08:00) Asia/Singapore",
                                        "(UTC+08:00) Asia/Taipei",
                                        "(UTC+08:00) Asia/Irkutsk",
                                        "(UTC+08:00) Asia/Ulaanbaatar",
                                        "(UTC+09:00) Asia/Seoul",
                                        "(UTC+09:00) Asia/Tokyo",
                                        "(UTC+09:00) Asia/Yakutsk",
                                        "(UTC+10:00) Asia/Vladivostok",
                                        "(UTC+11:00) Asia/Magadan",
                                        "(UTC+11:00) Asia/Srednekolymsk",
                                        "(UTC+12:00) Asia/Kamchatka" };

        const char *atlanticZones[] = { "(UTC-02:00) Atlantic/South_Georgia",
                                        "(UTC-01:00) Atlantic/Azores",
                                        "(UTC-01:00) Atlantic/Cape_Verde" };

        const char *austraZones[]   = { "(UTC+08:00) Australia/Perth",
                                        "(UTC+09:30) Australia/Darwin",
                                        "(UTC+09:30) Australia/Adelaide",
                                        "(UTC+10:00) Australia/Melbourne",
                                        "(UTC+10:00) Australia/Sydney",
                                        "(UTC+10:00) Australia/Brisbane",
                                        "(UTC+10:00) Australia/Hobart" };

        const char *europeZones[]   = { "(UTC+00:00) Europe/Dublin",
                                        "(UTC+00:00) Europe/London",
                                        "(UTC+00:00) Europe/Lisbon",
                                        "(UTC+01:00) Europe/Belgrade",
                                        "(UTC+01:00) Europe/Bratislava",
                                        "(UTC+01:00) Europe/Budapest",
                                        "(UTC+01:00) Europe/Ljubljana",
                                        "(UTC+01:00) Europe/Prague",
                                        "(UTC+01:00) Europe/Sarajevo",
                                        "(UTC+01:00) Europe/Skopje",
                                        "(UTC+01:00) Europe/Warsaw",
                                        "(UTC+01:00) Europe/Zagreb",
                                        "(UTC+01:00) Europe/Brussels",
                                        "(UTC+01:00) Europe/Copenhagen",
                                        "(UTC+01:00) Europe/Madrid",
                                        "(UTC+01:00) Europe/Paris",
                                        "(UTC+01:00) Europe/Amsterdam",
                                        "(UTC+01:00) Europe/Berlin",
                                        "(UTC+01:00) Europe/Rome",
                                        "(UTC+01:00) Europe/Stockholm",
                                        "(UTC+01:00) Europe/Vienna",
                                        "(UTC+02:00) Europe/Bucharest",
                                        "(UTC+02:00) Europe/Helsinki",
                                        "(UTC+02:00) Europe/Kiev",
                                        "(UTC+02:00) Europe/Riga",
                                        "(UTC+02:00) Europe/Sofia",
                                        "(UTC+02:00) Europe/Tallinn",
                                        "(UTC+02:00) Europe/Vilnius",
                                        "(UTC+02:00) Europe/Athens",
                                        "(UTC+03:00) Europe/Istanbul",
                                        "(UTC+03:00) Europe/Minsk",
                                        "(UTC+02:00) Europe/Kaliningrad",
                                        "(UTC+03:00) Europe/Moscow",
                                        "(UTC+03:00) Europe/Volgograd",
                                        "(UTC+04:00) Europe/Samara" };

        const char *pacificZones[]  = { "(UTC+10:00) Pacific/Guam",
                                        "(UTC+10:00) Pacific/Port_Moresby",
                                        "(UTC+11:00) Pacific/Guadalcanal",
                                        "(UTC+11:00) Pacific/Noumea",
                                        "(UTC+12:00) Pacific/Fiji",
                                        "(UTC+12:00) Pacific/Majuro",
                                        "(UTC+12:00) Pacific/Auckland",
                                        "(UTC+13:00) Pacific/Tongatapu",
                                        "(UTC+13:00) Pacific/Fakaofo",
                                        "(UTC+12:45) Pacific/Chatham",
                                        "(UTC-11:00) Pacific/Apia",
                                        "(UTC-11:00) Pacific/Midway",
                                        "(UTC-11:00) Pacific/Pago_Pago",
                                        "(UTC-10:00) Pacific/Honolulu" };

        const char *etcZones[]      = { "(UTC+00:00) Etc/UTC" };

        // Create the continent/timezone item and add it to the validTimeZone
        // vector
        timeZoneItem tzi;

        // Push African time zones
        tzi.first = "Africa";
        tzi.second.assign(africaZones, africaZones + (sizeof(africaZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push American time zones
        tzi.first = "Americas";
        tzi.second.assign(americaZones, americaZones + (sizeof(americaZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Asian time zones
        tzi.first = "Asia";
        tzi.second.assign(asiaZones, asiaZones + (sizeof(asiaZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Atlantic time zones
        tzi.first = "Atlantic Ocean";
        tzi.second.assign(atlanticZones, atlanticZones + (sizeof(atlanticZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Australian time zones
        tzi.first = "Australia";
        tzi.second.assign(austraZones, austraZones + (sizeof(austraZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Europe time zones
        tzi.first = "Europe";
        tzi.second.assign(europeZones, europeZones + (sizeof(europeZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Pacific time zones
        tzi.first = "Pacific Ocean";
        tzi.second.assign(pacificZones, pacificZones + (sizeof(pacificZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);

        // Push Etc time zones
        tzi.first = "Etc";
        tzi.second.assign(etcZones, etcZones + (sizeof(etcZones) / sizeof(const char *)));
        m_validTimeZones.push_back(tzi);
    }
};

#endif /* endif CLI_TIME_H */

