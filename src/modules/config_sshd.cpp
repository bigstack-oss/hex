// HEX SDK

#include <errno.h>
#include <ifaddrs.h> // getifaddrs()
#include <net/if.h> // IFF_UP
#include <arpa/inet.h> // inet_ntop()

#include <vector>

#include <hex/log.h>
#include <hex/parse.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/pidfile.h>
#include <hex/strict.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

static const char SSH_NAME[] = "sshd";

typedef std::pair<std::string, std::string> SSHSetting;
typedef std::vector<SSHSetting> SSHSettingList;

static SSHSettingList s_defSettings;
static SSHSettingList s_strictSettings;

static const char CONFFILE[] = "/etc/ssh/sshd_config";
static const char PIDFILE[] = "/var/run/sshd.pid";
static const char DISPLAY_NAME[] = "ssh daemon";

static const char rsaHostKey[]="/etc/ssh/ssh_host_rsa_key";
static const char rsaHostKeyPub[]="/etc/ssh/ssh_host_rsa_key.pub";
static const char ecDsaHostKey[]="/etc/ssh/ssh_host_ecdsa_key";
static const char ecDsaHostKeyPub[]="/etc/ssh/ssh_host_ecdsa_key.pub";
// strict mode non-conformed key pairs
static const char dsaHostKey[]="/etc/ssh/ssh_host_dsa_key";
static const char dsaHostKeyPub[]="/etc/ssh/ssh_host_dsa_key.pub";
static const char edHostKey[]="/etc/ssh/ssh_host_ed25519_key";
static const char edHostKeyPub[]="/etc/ssh/ssh_host_ed25519_key.pub";

static int s_rsaKeySize = 2048;
static int s_ecDsaKeySize = 521;

// private tunings
CONFIG_TUNING_BOOL(SSHD_ENABLED, "sshd.enabled", TUNING_UNPUB, "Set to true to enable SSH server.", true);

// public tunings
CONFIG_TUNING_BOOL(SSHD_BIND_ALL_INTERFACE, "sshd.bind_to_all_interfaces", TUNING_PUB, "Set to true to bind SSH server IP/Port on all network interfaces.", true);
CONFIG_TUNING_INT(SSHD_SESSION_TIMEOUT, "sshd.session.inactivity", TUNING_PUB, "Set SSH session idle timeout.", 0, 0, UINT16_MAX);

// parse tunings
PARSE_TUNING_BOOL(s_enabled, SSHD_ENABLED);
PARSE_TUNING_INT(s_sessionTimeout, SSHD_SESSION_TIMEOUT);
PARSE_TUNING_BOOL(s_bind2AllIf, SSHD_BIND_ALL_INTERFACE);
PARSE_TUNING_STR(s_defIf, NET_DEFAULT_INTERFACE);

static bool
Init()
{
    s_defSettings.clear();
    s_strictSettings.clear();

    // connections
    std::stringstream timeout_sstr;
    timeout_sstr << s_sessionTimeout * 60;
    s_defSettings.push_back(std::make_pair("Port","22"));
    s_defSettings.push_back(std::make_pair("Protocol","2"));
    s_defSettings.push_back(std::make_pair("ClientAliveCountMax","0"));
    s_defSettings.push_back(std::make_pair("ClientAliveInterval", timeout_sstr.str()));

    // authentication
    s_defSettings.push_back(std::make_pair("UsePAM","yes"));
    s_defSettings.push_back(std::make_pair("MaxAuthTries","4"));
    s_defSettings.push_back(std::make_pair("ChallengeResponseAuthentication","yes"));
    s_defSettings.push_back(std::make_pair("PermitRootLogin","yes"));
    s_defSettings.push_back(std::make_pair("PasswordAuthentication","yes"));

    // CBC mode ciphers and weak MAC algorithms (MD5 and -96) should be disbaled
    s_defSettings.push_back(std::make_pair("Ciphers","aes128-ctr,aes192-ctr,aes256-ctr"));
    s_defSettings.push_back(std::make_pair("MACs","hmac-sha1,hmac-sha2-256,hmac-sha2-512"));

    // config to allow sftp
    s_defSettings.push_back(std::make_pair("Subsystem sftp","internal-sftp"));
    s_defSettings.push_back(std::make_pair("Match User","admin"));
    s_defSettings.push_back(std::make_pair("ForceCommand","NO_SFTP"));
    s_defSettings.push_back(std::make_pair("Match Group","admin"));
    s_defSettings.push_back(std::make_pair("ForceCommand","NO_SFTP"));

    s_strictSettings.push_back(std::make_pair("ReKeyLimit","1G 3600"));
    s_strictSettings.push_back(std::make_pair("KexAlgorithms","diffie-hellman-group14-sha1"));

    return true;
}

static bool
Parse(const char *name, const char *value, bool isNew)
{
    bool r = true;

    TuneStatus s = ParseTune(name, value, isNew);
    if (s == TUNE_INVALID_VALUE) {
        HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
        r = false;
    }
    return r;
}

static void
CheckKeys(const char* keyname, const char* pubkeyname)
{
    HexLogDebug("(config_sshd): Checking key: %s", keyname);

    // Remove keys that are less than 2048-bits so
    // that they will be regenerated as 2048-bit
    // (except for ECDSA keys that are 521-bits)

    bool sizeOK = false;
    char buffer[1024];
    int keySize = -1, keyReq = -1;
    // Look for the comment line that displays key size, e.g:
    // Comment: "2048-bit RSA,..." or "521-bit ECDSA,..."
    const char *searchStr = "Comment: \"";
    FILE *fd = HexPOpenF("/usr/bin/ssh-keygen -e -f %s", keyname);
    if (fd) {
        while (fgets(buffer, sizeof(buffer), fd)) {
            char *pos;
            if ((pos = strstr(buffer, searchStr))) {
                char *keySizeStr = pos + strlen(searchStr);
                if ((pos = strchr(keySizeStr, '"'))) {
                    *pos = '\0';
                }

                keySize = atoi(keySizeStr);
                if (strstr(keySizeStr, " ECDSA,"))
                    keyReq = s_ecDsaKeySize;
                else
                    keyReq = s_rsaKeySize;

                if (keySize >= keyReq)
                    sizeOK = true;

                break;
            }
        }
        fclose(fd);
    }

    if (!sizeOK) {
        HexLogNotice("config_sshd: SSH key %s with key size %d does not meet required minimum key size of %d. Deleting key.",
                      keyname, keySize, keyReq);
        if (HexSystem(0, "rm -f", keyname, (const char *) 0) == 0) {
            HexSystem(0, "rm -f", pubkeyname, (const char *) 0);
        }
        else {
            HexLogError("Unable to delete old SSH keys");
        }
    }
}

static void
CreateKeys()
{
    int strictEnabled = HexStrictIsEnabled();

    /* For the SSH2 protocol you need two keys, for rsa and dsa */

    // Check the size of RSA key pair
    if (access(rsaHostKey,F_OK) == 0) {
        CheckKeys(rsaHostKey, rsaHostKeyPub);
    }
    // Generate RSA key pair if it doesn't exist
    if (access(rsaHostKey,F_OK)) {
        HexLogNotice("config_sshd: Generating %s", rsaHostKey);
        if (HexSystemF(0, "/usr/bin/ssh-keygen -q -t rsa -b %d -f %s -N '' </dev/null >/dev/null 2>&1", s_rsaKeySize, rsaHostKey) != 0) {
            HexLogError("failed to generate %s", rsaHostKey);
        }
    }

    // Since ECDSA is using elliptic curve, FIPS mandates >= 224-bits:
    // http://csrc.nist.gov/publications/nistpubs/800-131A/sp800-131A.pdf (page 5, table 2)
    // Check the size of ECDSA key pair
    if (access(ecDsaHostKey,F_OK) == 0) {
        CheckKeys(ecDsaHostKey, ecDsaHostKeyPub);
    }
    // Generate ECDSA key pair if it doesn't exist
    if (access(ecDsaHostKey,F_OK)) {
        HexLogNotice("config_sshd: Generating %s", ecDsaHostKey);
        if (HexSystemF(0, "/usr/bin/ssh-keygen -q -t ecdsa -b %d -f %s -N '' </dev/null >/dev/null 2>&1", s_ecDsaKeySize, ecDsaHostKey) != 0) {
            HexLogError("failed to generate %s", ecDsaHostKey);
        }
    }

    // SSH DSA keys are 1024-bits so we cannot use in FIPS/NIST mode
    if (strictEnabled) {
        // Check the size of DSA key pair
        if (access(dsaHostKey,F_OK) == 0) {
            CheckKeys(dsaHostKey, dsaHostKeyPub);
        }
        // Check the size of DSA key pair
        if (access(edHostKey,F_OK) == 0) {
            CheckKeys(edHostKey, edHostKeyPub);
        }
    }
    else {
        // Generate DSA key pair if it doesn't exist
        if (access(dsaHostKey,F_OK)) {
            HexLogNotice("config_sshd: Generating %s", dsaHostKey);
            if (HexSystemF(0, "/usr/bin/ssh-keygen -q -t dsa -b 1024 -f %s -N '' </dev/null >/dev/null 2>&1", dsaHostKey) != 0) {
                HexLogError("failed to generate %s", dsaHostKey);
            }
        }

        // Generate ED key pair if it doesn't exist
        if (access(edHostKey,F_OK)) {
            HexLogNotice("config_sshd: Generating %s", edHostKey);
            if (HexSystemF(0, "/usr/bin/ssh-keygen -q -t ed25519 -f %s -N '' </dev/null >/dev/null 2>&1", edHostKey) != 0) {
                HexLogError("failed to generate %s", edHostKey);
            }
        }
    }
}

static void
ReCreateKeys()
{
    // Remove every key then create them later
    if (access(rsaHostKey,F_OK) == 0) {
        HexSystem(0, "rm -f", rsaHostKey, (const char *) 0);
        HexSystem(0, "rm -f", rsaHostKeyPub, (const char *) 0);
    }

    if (access(ecDsaHostKey,F_OK) == 0) {
        HexSystem(0, "rm -f", ecDsaHostKey, (const char *) 0);
        HexSystem(0, "rm -f", ecDsaHostKeyPub, (const char *) 0);
    }

    if (access(dsaHostKey,F_OK) == 0) {
        HexSystem(0, "rm -f", dsaHostKey, (const char *) 0);
        HexSystem(0, "rm -f", dsaHostKeyPub, (const char *) 0);
    }

    if (access(edHostKey,F_OK) == 0) {
        HexSystem(0, "rm -f", edHostKey, (const char *) 0);
        HexSystem(0, "rm -f", edHostKeyPub, (const char *) 0);
    }

    CreateKeys();
}

// Update the interfaces part of the sshd configuration file so that we only
// listen on the desired interfaces. The return value is used to indicate
// whether the interfaces have been updated or not.
static bool
UpdateConfig()
{
    // If we're binding to all interfaces, or STRICT is not enabled,
    // just use the default sshd_config file
    if (s_bind2AllIf && !HexStrictIsEnabled())
        return false;

    // Back-up the current sshd configuration file.  We use this later on
    // to determine whether the files have changed.

    std::string cfgFile_backup;

    cfgFile_backup  = CONFFILE;
    cfgFile_backup += ".bak";

    unlink(cfgFile_backup.c_str());

    if (access(CONFFILE, F_OK) == 0) {
        if (HexSystem(0, "cp", CONFFILE, cfgFile_backup.c_str(), (const char *) 0) != 0 ) {
            HexLogFatal("Could not back-up %s", CONFFILE);
        }
    }

    // Remove any existing Listen statements from the configuration file.
    const char* pcMod = "'/^ListenAddress.*/d'";
    if (HexSystem(0, "sed", "-r", "-e", pcMod, "-i", CONFFILE, (const char *) 0) != 0 ) {
        HexLogFatal("Removing listen address within %s failed", CONFFILE);
    }

    // Remove the existing log settings
    HexSystem(0, "sed", "-r", "-e", "'/^LogLevel.*/d'", "-e", "'/^SyslogFacility.*/d'", "-i", CONFFILE, (const char *) 0);

    // Open the file to append new configuration
    FILE* fout = fopen(CONFFILE, "a");
    if (!fout) {
        HexLogFatal("Could not open %s for writing", CONFFILE);
    }

    // Now we need to retrieve a list of all of our interfaces, and add the
    // IP adddress for those interfaces to our sshd configuration file.

    struct ifaddrs *myaddrs, *ifa;
    char buf[64]; // buf must be big enough to hold an IPv6 address.

    // Get the list of our IP addresses.
    int ret = getifaddrs(&myaddrs);
    if (ret != 0) {
        HexLogWarning("config_sshd: Cannot retrieve the IP addresses of the appliance, errno=%d", errno);
        fclose(fout);
        return false;
    }

    // Scroll through each of our interfaces, looking for a match with our
    // list of management interfaces.
    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next) {

        // Is this a valid interface?
        if (ifa->ifa_addr == NULL)          continue;
        if ((ifa->ifa_flags & IFF_UP) == 0) continue;

        // Is this the default interface
        if (s_defIf.c_str() != ifa->ifa_name) continue;

        // Obtain the IP address of the interfaces and then add it to our list of interfaces.
        std::string ip;

        if (ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* s4 = (struct sockaddr_in *)(ifa->ifa_addr);
            if (inet_ntop(ifa->ifa_addr->sa_family,
                (void *)&(s4->sin_addr), buf, sizeof(buf)) == NULL) {
                HexLogWarning("inet_ntop failed");
                continue;
            }

            ip = buf;
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
            if (inet_ntop(ifa->ifa_addr->sa_family,
                (void *)&(s6->sin6_addr), buf, sizeof(buf)) == NULL) {
                HexLogWarning("inet_ntop failed");
                continue;
            }

            // Check to see that we can successfully bind to this address.  We
            // will not be able to bind to the address while it is in the
            // 'tentative' state.
            int s = socket(AF_INET6, SOCK_STREAM, 0);
            if (s == -1 || bind(s, (sockaddr*)s6, sizeof(struct sockaddr_in6)) != 0) {
                HexLogDebug("The address is not currently available: %s", buf);
                continue;
            }

            close(s);

            ip = buf;

            // If this is a link local IPv6 address we also need to append
            // the interface name to the end of the address so that the
            // OS can correctly scope this address.

            if (s6->sin6_scope_id != 0) {
                ip += "%";
                ip += ifa->ifa_name;
            }
        }

        if (ip.length() > 0) {
            HexLogDebug("Found a IP address for default interface: %s", ip.data());
            fprintf(fout, "ListenAddress %s\n", ip.data());
        }
    }

    if (myaddrs) freeifaddrs(myaddrs);

    // Enable info logging.
    fprintf(fout, "SyslogFacility AUTH\n");
    fprintf(fout, "LogLevel INFO\n");

    fclose(fout);

    // We now return whether we have actually modified the configuration or not.
    return (HexSystem(0, "diff", "-q", "-w", CONFFILE, cfgFile_backup.data(), "2>&1 >", "/dev/null", NULL) != 0);
}

static void
RemoveSetting(std::string key,std::string config_file)
{
    char settingMod[512]={};
    snprintf(settingMod, sizeof(settingMod), "-e '/.*%s.*/d'", key.c_str());
    if (HexSystem(0, "sed", "-r", settingMod, "-i", config_file.c_str(), (const char *) 0) != 0 ) {
        HexLogFatal("Removing %s within %s failed", key.c_str(), config_file.c_str());
    }
}

static void
AddSetting(std::string key, std::string value, std::string config_file)
{
    if (HexSystemF(0, "echo '%s %s' >> %s", key.c_str(), value.c_str(), config_file.c_str()) != 0 ) {
        HexLogFatal("Inserting config %s:%s within %s failed", key.c_str(), value.c_str(), config_file.c_str());
    }
}

static bool
Commit(bool modified, int dryLevel)
{
    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!UpdateConfig() && !modified)
        return true;

    int strictEnabled = HexStrictIsEnabled();

    // Load the SSH settings
    Init();

    for (SSHSettingList::const_iterator iter=s_strictSettings.begin() ; iter!=s_strictSettings.end() ; ++iter) {
        RemoveSetting(iter->first.c_str(), CONFFILE);
        // Apply strict settings only in STRICT mode
        if (strictEnabled) {
            AddSetting(iter->first.c_str(), iter->second.c_str(), CONFFILE);
        }
    }

    // Add the default settings for sshd
    for (SSHSettingList::const_iterator iter=s_defSettings.begin() ; iter!=s_defSettings.end() ; ++iter) {
        RemoveSetting(iter->first.c_str(), CONFFILE);
    }

    // make sure default subsystem is removed
    if ( HexSystem(0, "sed", "-i", "-e", "s/^Subsystem/#Subsystem/", CONFFILE, (const char *) 0) != 0 ) {
        HexLogFatal("Removing default subsystem within %s failed", CONFFILE);
    }

    for (SSHSettingList::const_iterator iter=s_defSettings.begin() ; iter!=s_defSettings.end() ; ++iter) {
        AddSetting(iter->first.c_str(), iter->second.c_str(), CONFFILE);
    }

    // Stop daemon if running
    // (will be restarted if necessary)
    HexUtilSystemF(FWD, 0, "systemctl stop %s", SSH_NAME);

    if (s_enabled) {
        HexLogDebug("starting %s service", SSH_NAME);

        CreateKeys();
        int status = HexUtilSystemF(0, 0, "systemctl start %s", SSH_NAME);
        if (status != 0) {
            HexLogError("failed to start %s", SSH_NAME);
        }

        HexLogInfo("%s is running", SSH_NAME);
    }
    else {
        HexLogInfo("%s has been stopped", SSH_NAME);
    }

    return true;
}

static int
Refresh(int argc, char **argv)
{
    Commit(false, 0);
    return 0;
}

static void
GenerateKeysUsage(void)
{
    fprintf(stderr, "Usage: %s generate_ssh_keys\n", HexLogProgramName());
}

static int
GenerateKeysMain(int argc, char **argv)
{
    if (argc != 1) {
        GenerateKeysUsage();
        return 1;
    }
    CreateKeys();
    return 0;
}

static void
ReGenerateKeysUsage(void)
{
    fprintf(stderr, "Usage: %s regenerate_ssh_keys\n", HexLogProgramName());
}

static int
ReGenerateKeysMain(int argc, char **argv)
{
    if (argc != 1) {
        ReGenerateKeysUsage();
        return 1;
    }
    ReCreateKeys();
    Commit(true, 0);
    return 0;
}

CONFIG_MODULE(sshd, Init, Parse, NULL, NULL, Commit);

CONFIG_OBSERVES(sshd, net, Parse, 0);

CONFIG_REQUIRES(sshd, net_static);

CONFIG_SHUTDOWN(sshd, PIDFILE);

CONFIG_TRIGGER_WITH_SETTINGS(sshd, "dhcp_lease_renewed",       Refresh);
CONFIG_TRIGGER_WITH_SETTINGS(sshd, "ipv6_auto_address_active", Refresh);

// Preserve ssh server identity across firmware updates
CONFIG_MIGRATE(sshd, "/etc/ssh/ssh_host_rsa_key*");
CONFIG_MIGRATE(sshd, "/etc/ssh/ssh_host_ecdsa_key*");
CONFIG_MIGRATE(sshd, "/etc/ssh/ssh_host_dsa_key*");
CONFIG_MIGRATE(sshd, "/etc/ssh/ssh_host_ed25519_key*");

// CAUTION! Don't put customer keys in a support info
CONFIG_SUPPORT_FILE("/etc/ssh/sshd_config");

// Files to zeroize in case of STRICT error
CONFIG_STRICT_ZEROIZE(rsaHostKey);
CONFIG_STRICT_ZEROIZE(ecDsaHostKey);
CONFIG_STRICT_ZEROIZE(dsaHostKey);
CONFIG_STRICT_ZEROIZE(edHostKey);

CONFIG_COMMAND(generate_ssh_keys, GenerateKeysMain, GenerateKeysUsage);
CONFIG_COMMAND(regenerate_ssh_keys, ReGenerateKeysMain, ReGenerateKeysUsage);

