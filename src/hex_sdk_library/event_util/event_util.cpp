// HEX SDK

#include <libintl.h>

#include <hex/log.h>
#include <hex/string_util.h>
#include <hex/event_util.h>

#define DNAME "event"
#define CATALOGS_DIR "/var/catalog"

bool
HexParseEvent(const std::string &message, std::string &eventid, std::string &args)
{
    const char *eidEnd = 0;

    // e.g. "eventid:: |arg0=val1,arg1=val2,...|"
    if ((eidEnd = strstr(message.c_str(), ":: ")) != NULL) {
        // parse the event id
        eventid = std::string(message.c_str(), eidEnd - message.c_str());

        // parse the args
        const char *pipestart = eidEnd + strlen(":: ");
        const char *pipeend = rindex(pipestart + 1, '|');
        if (pipeend != NULL) {
            args = std::string(pipestart + 1, pipeend - (pipestart + 1));
            return true;
        }
    }

    return false;
}

bool
HexParseEventArgs(const std::string &args, std::map<std::string, std::string> &argMap)
{
    // separate with comma
    std::vector<std::string> argv = hex_string_util::split(args, ',');

    for (unsigned int idx = 0; idx < argv.size(); ++idx) {
        std::vector<std::string> pair = hex_string_util::split(argv[idx], '=');
        if (pair.size() != 2)
            continue;

        char buf[512];
        snprintf(buf, sizeof(buf), "%s", pair[1].c_str());
        HexLogUnescape(buf);
        argMap[pair[0]] = buf;
    }

    return true;
}

char*
HexLookupEventText(const char *eventid, const char *args, const char *locale)
{
    if (!eventid || !locale) {
        return NULL;
    }

    std::map<std::string, std::string> argMap;
    // parse the event args
    if (args) {
        HexParseEventArgs(args, argMap);
        if (argMap.size() == 0)
            return NULL;
    }

    setlocale(LC_ALL, locale);
    bindtextdomain(DNAME, CATALOGS_DIR);
    bind_textdomain_codeset(DNAME, "utf-8");
    textdomain(DNAME);

    std::string tStr = std::string(gettext(eventid));

    for (auto arg = argMap.begin() ; arg != argMap.end(); arg++) {
        std::string from = "{{" + arg->first + "}}";
        std::string to = arg->second;

        // replace "{{key}}" to "value"
        hex_string_util::replace(tStr, from, to);
    }

    char *buf = (char *)malloc(tStr.length() + 1);
    if (buf == NULL) {
        HexLogWarning("could not allocate buffer for tranlsated text");
        return NULL;
    }

    snprintf(buf, tStr.length() + 1, "%s", tStr.c_str());

    return buf;
}

