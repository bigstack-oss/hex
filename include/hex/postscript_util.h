// HEX SDK

#ifndef HEX_POSTSCRIPT_UTIL_H
#define HEX_POSTSCRIPT_UTIL_H

#ifdef __cplusplus

#include <vector>

#include <unistd.h>
#include <hex/log.h>
#include <hex/process.h>

/*
 * Execute post script associating with the executed hex command and sub-command
 * Scripts are located at: /etc/<command>/post.d/<sub-command>.post.sh
 * 
 * Ex. The post script for "hex_config apply" is:
 *  /etc/hex_config/post.d/apply.post.sh
 */
static inline int
HexPostScriptExec(int argc, char **argv, const std::string &dir)
{
    std::string scriptPath = dir + "/" + argv[0] + ".post.sh";
    if (access(scriptPath.c_str(), F_OK) != 0) {
        HexLogDebug("No post script found: %s", scriptPath.c_str());
        return EXIT_SUCCESS;
    }

    std::vector<const char *> args;
    args.push_back("/bin/sh");
    args.push_back(scriptPath.c_str());
    args.insert(args.end(), argv + 1, argv + argc);
    args.push_back(NULL);

    int ret = HexSpawnV(0, (char *const *)&args[0]);
    HexLogInfo("Executed post script: %s, return code: %d", scriptPath.c_str(), ret);

    return ret;
}


#endif /* __cplusplus */

#endif /* endif HEX_POSTSCRIPT_UTIL_H */