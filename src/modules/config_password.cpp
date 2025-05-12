// HEX SDK

#include <pwd.h>    // getpwnam_r
#include <unistd.h> // pipe, fork
#include <errno.h>  // errno
#include <shadow.h> // getspnam

#include <hex/log.h>
#include <hex/zeroize.h>
#include <hex/process.h>
#include <hex/process_util.h>

#include <hex/config_module.h>
#include <hex/config_tuning.h>
#include <hex/dryrun.h>

#define MAX_LINEBUFF_LEN 1024
#define DEFAULT_MIN_PASSWD_LEN   6
#define SECS_PER_DAY 86400

typedef std::vector<std::pair<std::string, std::string> > NvpList;

static const char* ADMIN_USER ="admin";

static const char* CHAGE = "/usr/bin/chage";    // change user password expiry information
static const char PASSWD[] = "/usr/bin/passwd"; // change user password

static const char AUTH_SETTINGS[]   = "/etc/pam.d/system-auth";
static const char TEMP_AUTH_SETTINGS[]   = "/etc/pam.d/system-auth.tmp";

// pam_unix_passwd
static const char PAM_UNIX_PWD_LINE[] = "password    required       pam_unix_passwd.so   sha512";

// pam_cracklib give application to provide some plug-in strength-checking for passwords.
static const char PAM_CRACKLIB_LINE[] = "password    required       pam_cracklib.so      ";

// pam_tally2 maintains a count of attempted accesses,
// can reset count on success, can deny access if too many attempts fail
static const char AUTH_PAM_TALLY_LINE[] = "auth    required        pam_tally2.so  ";
static const char ACCOUNT_PAM_TALLY_LINE[] = "account required        pam_tally2.so";

static const char PAM_SSHD_SETTINGS[]      = "/etc/pam.d/sshd";
static const char TEMP_PAM_SSHD_SETTINGS[]      = "/etc/pam.d/sshd.tmp";

static const char CRON_FILE_FMT[] = "/etc/cron.d/check_%s_password";

//currently, the only user is admin
CONFIG_TUNING_BOOL(PASSWORD_ENABLE_COMPLEXITY, "password.enable_complexity", TUNING_UNPUB, "Enable complex policy rules.", false);
CONFIG_TUNING_INT(PASSWORD_MINLEN, "password.minlen", TUNING_UNPUB, "Set minimum password length.", 6, 6, 128);
CONFIG_TUNING_INT(PASSWORD_MAXDAYS, "password.max_days", TUNING_UNPUB, "Set maximum number of days between password change.", 0, -1, 99999);
CONFIG_TUNING_INT(PASSWORD_MINDAYS, "password.min_days", TUNING_UNPUB, "Set minimum number of days between password change.", 0, 0, 99999);
CONFIG_TUNING_INT(PASSWORD_REMEMBER, "password.remember", TUNING_UNPUB, "Set the number of password history.", 0, 0, 99999);
CONFIG_TUNING_INT(PASSWORD_MAXFAIL, "password.maxfail", TUNING_UNPUB, "Set maximum failures to try password.", 0, 0, 99999);
CONFIG_TUNING_INT(PASSWORD_LOCKOUT_TIME, "password.lockouttime", TUNING_UNPUB, "Set the time to be locked after reached maxfail.", 0, 0, 99999);
CONFIG_TUNING_BOOL(PASSWORD_OCR, "password.ocredit", TUNING_UNPUB, "Set to required other letters.", false);
CONFIG_TUNING_BOOL(PASSWORD_DCR, "password.dcredit", TUNING_UNPUB, "Set to required digits.", false);
CONFIG_TUNING_BOOL(PASSWORD_UCR, "password.ucredit", TUNING_UNPUB, "Set to required uppercase letters.", false);

// parse tunings
PARSE_TUNING_BOOL(s_enable_complexity, PASSWORD_ENABLE_COMPLEXITY);
PARSE_TUNING_INT(s_minlen, PASSWORD_MINLEN);
PARSE_TUNING_INT(s_max_days, PASSWORD_MAXDAYS);
PARSE_TUNING_INT(s_min_days, PASSWORD_MINDAYS);
PARSE_TUNING_INT(s_remember, PASSWORD_REMEMBER);
PARSE_TUNING_INT(s_maxfail, PASSWORD_MAXFAIL);
PARSE_TUNING_INT(s_lockouttime, PASSWORD_LOCKOUT_TIME);
PARSE_TUNING_BOOL(s_ocredit, PASSWORD_OCR);
PARSE_TUNING_BOOL(s_ucredit, PASSWORD_DCR);
PARSE_TUNING_BOOL(s_dcredit, PASSWORD_UCR);

static bool AddCheckPasswordJob(const char *username)
{
    char cronfile[128]={};
    char check_command[256]={};
    snprintf(cronfile, sizeof(cronfile), CRON_FILE_FMT, username);
    snprintf(check_command, sizeof(check_command), HEX_CFG " check_password %s", username);

    HexLogDebug("Adding crob job %s", cronfile);
    FILE *fout = fopen(cronfile, "w");
    if (!fout) {
        HexLogError("Could not write file: %s", cronfile);
        return false;
    }

    // Check subscription password expirations once per day
    fprintf(fout, "0 0 * * * %s\n", check_command);
    fclose(fout);

    return true;
}

static int
RemoveSetting(std::string key,std::string config_file)
{
    int ret = 0;
    char settingMod[512] = {};

    snprintf(settingMod, sizeof(settingMod), "-e '/.*%s.*/d'", key.c_str());
    if ((ret = HexSystem(0, "sed", "-r", settingMod, "-i", config_file.c_str(), (const char *) 0)) != 0 ) {
        HexLogError("Removing %s within %s failed", key.c_str(), config_file.c_str());
    }

    return ret;
}

static bool
SaveTMPPWDSettings()
{
    std::ofstream ofs;

    if (RemoveSetting("password.*pam_cracklib.so", TEMP_AUTH_SETTINGS)!=0 ||
        RemoveSetting("password.*pam_unix_passwd.so", TEMP_AUTH_SETTINGS)!=0)
        return false;

    ofs.open(TEMP_AUTH_SETTINGS, std::ofstream::out | std::ofstream::app);

    if (!ofs.good())
        return false;

    if (s_enable_complexity) {

        ofs << PAM_CRACKLIB_LINE;
        ofs << "minlen=" << s_minlen.newValue() << " ";

        if (s_ocredit) {
            ofs << "ocredit=-1 ";
        }
        else {
            ofs << "ocredit=0 ";
        }

        if (s_dcredit) {
            ofs << "dcredit=-1 ";
        }
        else {
            ofs << "dcredit=0 ";
        }

        if (s_ucredit) {
            ofs << "ucredit=-1 " << "lcredit=-1 ";
        }
        else {
            ofs << "ucredit=0 " << "lcredit=0 ";
        }
        ofs << std::endl;
    }

    ofs << PAM_UNIX_PWD_LINE;

    if (s_enable_complexity)
        ofs << " use_authtok";

    if (s_remember!=0)
        ofs << " remember=" << s_remember;

    ofs << std::endl;
    ofs.close();

    return true;
}

static bool
SaveTMPAuthSettings()
{
    if (RemoveSetting("auth.*pam_tally2.so", TEMP_AUTH_SETTINGS)!=0 )
        return false;

    if (s_maxfail!=0) {
        std::ofstream ofs;
        ofs.open(TEMP_AUTH_SETTINGS, std::ofstream::out | std::ofstream::app);
        if (!ofs.good())
            return false;
        ofs << AUTH_PAM_TALLY_LINE << "deny=" << s_maxfail.newValue() << "  unlock_time=" << s_lockouttime.newValue()*60 << std::endl;

        ofs.close();
    }

    return true;
}

static bool
SaveTMPAccountSettings()
{
    if (RemoveSetting("account.*pam_tally2.so", TEMP_AUTH_SETTINGS)!=0 )
        return false;

    if (s_maxfail!=0) {
        std::ofstream ofs;
        ofs.open(TEMP_AUTH_SETTINGS, std::ofstream::out | std::ofstream::app);
        if (!ofs.good())
            return false;
        ofs << ACCOUNT_PAM_TALLY_LINE << std::endl;

        ofs.close();
    }

    return true;
}

static bool SaveTMPSshdSettings()
{
    int rc = HexSystemF(0, "/bin/sed -i 's/auth.*substack.*password-auth/auth       substack     system-auth/' %s",
                           TEMP_PAM_SSHD_SETTINGS);
    if (rc!=0) {
        HexLogError("Failed to insert auth setting to %s", TEMP_PAM_SSHD_SETTINGS);
        return false;
    }

    rc = HexSystemF(0, "/bin/sed -i 's/password.*include.*password-auth/password   include      system-auth/' %s",
                       TEMP_PAM_SSHD_SETTINGS);
    if (rc!=0) {
        HexLogError("Failed to insert password setting to %s", TEMP_PAM_SSHD_SETTINGS);
        return false;
    }

    rc = HexSystemF(0, "/bin/sed -i 's/account.*include.*password-auth/account    include      system-auth/' %s",
                       TEMP_PAM_SSHD_SETTINGS);
    if (rc != 0) {
        HexLogError("Failed to insert account setting to %s", TEMP_PAM_SSHD_SETTINGS);
        return false;
    }

    return true;
}

static bool
Parse(const char* name, const char* value, bool isNew)
{
    bool r = true;

    HexLogInfo("PASSWORD PARSE");
    TuneStatus s = ParseTune(name, value, isNew);
    if (s == TUNE_INVALID_NAME) {
        HexLogWarning("Unknown settings name \"%s\" = \"%s\" ignored", name, value);
    }
    else if (s == TUNE_INVALID_VALUE) {
        HexLogError("Invalid settings value \"%s\" = \"%s\"", name, value);
        r = false;
    }
    return r;
}

static bool Commit(bool modified, int dryLevel)
{
    /* currently we only support the account "root" and "admin" for CLI
     * To support multiple administrator account creations,
     * it will need to update login.defs for max expired day and min expired day
     */

    // TODO: remove this if support dry run
    HEX_DRYRUN_BARRIER(dryLevel, true);

    if (!modified)
        return true;

    if (s_max_days.modified()) {
        // if s_max_days equals to 0, we want to disable max password lifetime check
        // We need to change max_days to -1 to actually disable it.
        if (s_max_days == 0)
            s_max_days = -1;

        // add a cron job to check expircy of admin password daily
        if (s_max_days != -1) {
            AddCheckPasswordJob(ADMIN_USER);
        }

        int rc =  HexSystemF(0, "%s %s -M %d", CHAGE, ADMIN_USER, static_cast<int>(s_max_days));
        if (rc != 0) {
            HexLogError("change max expired day to %d error: %s for %s", static_cast<int>(s_max_days), strerror(errno), ADMIN_USER);
        }
    }

    if (s_min_days.modified()) {
        if (s_max_days <= 0 || s_max_days > s_min_days) {
            int rc =  HexSystemF(0, "%s %s -m %d", CHAGE, ADMIN_USER, static_cast<int>(s_min_days));
            if (rc != 0) {
                HexLogError("change min expired day to %d error: %s for %s", static_cast<int>(s_min_days), strerror(errno), ADMIN_USER);
            }
        }
        else {
            HexLogError("min expired day(%d) cannot be smaller than max expired day(%d)", static_cast<int>(s_min_days), static_cast<int>(s_max_days));
        }
    }

    // one of them change will result to modify "/etc/pam.d/common-password"
    if (s_enable_complexity.modified() || s_minlen.modified() ||
        s_remember.modified() || s_ocredit.modified() ||
        s_dcredit.modified() || s_ucredit.modified()) {
        // dcredit=-1 for number char, ocredit=-1 for other char, ucredit=-1 for upper char, lcredit=-1 for lower char.
        if (HexSystem(0, "/bin/cp", "-f", AUTH_SETTINGS, TEMP_AUTH_SETTINGS, (const char*)0) == 0 &&
            SaveTMPPWDSettings()) {
            HexSystem(0, "/bin/cp", "-f", TEMP_AUTH_SETTINGS, AUTH_SETTINGS, (const char*)0);
        }
        HexSystem(0, "/bin/rm", "-f", TEMP_AUTH_SETTINGS, (const char*)0);
    }

    if (s_lockouttime.modified() || s_maxfail.modified()) {
        if (s_maxfail == 0)
            HexSystem(0, "/sbin/pam_tally2", "-r", (const char*)0);

        if (HexSystem(0, "/bin/cp", "-f", AUTH_SETTINGS, TEMP_AUTH_SETTINGS, (const char*)0)==0 &&
            HexSystem(0, "/bin/cp", "-f", AUTH_SETTINGS, TEMP_AUTH_SETTINGS, (const char*)0)==0 &&
            SaveTMPAuthSettings() && SaveTMPAccountSettings()) {
            HexSystem(0, "/bin/cp", "-f", TEMP_AUTH_SETTINGS, AUTH_SETTINGS, (const char*)0);
            HexSystem(0, "/bin/cp", "-f", TEMP_AUTH_SETTINGS, AUTH_SETTINGS, (const char*)0);
        }

        HexSystem(0, "/bin/rm", "-f", TEMP_AUTH_SETTINGS, (const char*)0);
        HexSystem(0, "/bin/rm", "-f", TEMP_AUTH_SETTINGS, (const char*)0);
    }

    // Make sure that the /etc/pam.d/sshd has all the settings we need
    if (HexSystem(0, "/bin/cp", "-f", PAM_SSHD_SETTINGS, TEMP_PAM_SSHD_SETTINGS, (const char*)0)==0 &&
        SaveTMPSshdSettings()) {
        HexSystem(0, "/bin/cp", "-f", TEMP_PAM_SSHD_SETTINGS, PAM_SSHD_SETTINGS, (const char*)0);
    }
    HexSystem(0, "/bin/rm", "-f", TEMP_PAM_SSHD_SETTINGS, (const char*)0);

    //Make sure the latest settings are flush to the disk
    sync();

    return true;
}

static int
GetUIDfromName(const char *user)
{
    struct passwd pwd;
    struct passwd *result;
    char *buf;
    long bufsize;
    int uid = -1;

    // sysconf(_SC_GETPW_R_SIZE_MAX) return -1 if there is no hard limit
    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1) {

        bufsize = 16384;
    }

    buf = (char *)malloc(bufsize);
    if (buf == NULL) {
        HexLogError("Can not allocate the required memory \n");
        return uid;
    }

    // search user database for a name
    getpwnam_r(user, &pwd, buf, bufsize, &result);
    if (result == NULL) {
        free(buf);
        HexLogError("Can not get %s UID \n", user);
        return uid;
    }

    uid = (int) pwd.pw_uid;
    free(buf);
    return uid;
}

//return value:
//0:  success,
//1:  failure,
//2:  password is too short,
//3:  still within PASS_MIN_DAYS
//4:  Password unchanged
//5:  Password is in password history and can not reuse.
//6:  BAD PASSWORD: an generic bad password message "the new password doesn't meet the password complexity"
//7:  BAD PASSWORD: is too similar to the old one
//8:  BAD PASSWORD: it is WAY too short
//9:  BAD PASSWORD: it is too short
//10: BAD PASSWORD: it is too simplistic/systematic
//11: BAD PASSWORD: it does not contain enough DIFFERENT characters
//12: BAD PASSWORD: it is based on a dictionary word
//13: BAD PASSWORD: it is based on your username
static int ChangeItsPassword(const char *user, const char *input, const char *newpassword)
{
    // We need to clearly tell if the new password is too short
    unsigned int min_pwd_len =
        s_enable_complexity ? static_cast<unsigned int>(s_minlen) : DEFAULT_MIN_PASSWD_LEN;

    if (strlen(newpassword) < min_pwd_len) {
        fprintf(stdout, "This value is too short (minimum is %d characters)\n", min_pwd_len);
        return 2;
    }

    char* args[5];
    int   fd1[2], fd2[2];
    pid_t pid;
    int  result = 1;

    // e.g. echo -e '123456\nabcdefg\nabcdefg' | passwd
    args[0] = (char *) PASSWD;
    args[1] = NULL;

    if (pipe(fd1) < 0 || pipe(fd2) < 0) {
        HexLogError("Unable to execute passwd utility for admin\n");
        return false;
    }

    pid = fork();
    if (pid == 0) // child (passwd command)
    {
        int uid = GetUIDfromName(user);

        setuid(uid);
        close(fd1[1]); // close read
        close(fd2[0]); // close write

        if (fd1[0] != STDIN_FILENO) {
            dup2(fd1[0], STDIN_FILENO);  // dup stdin to fd1[0]
            close(fd1[0]);
        }

        if (fd2[1] != STDOUT_FILENO) {
            dup2(fd2[1], STDOUT_FILENO); // dup stdout to fd2[1]
            dup2(fd2[1], STDERR_FILENO); // dup stderr to fd2[1]
            close(fd2[1]);
        }

        execv(args[0], args); // it's 'passwd' now
        exit(125);  // shouldn't get here
    }
    else {  // parent (pid > 0)
        int  status = -1;
        char buf[MAX_LINEBUFF_LEN + 1];
        char errmsg[256], *pos;
        FILE *fp[2];

        close(fd1[0]); // close write
        close(fd2[1]); // close read
        fp[0] = fdopen(fd2[0], "r");
        fp[1] = fdopen(fd1[1], "w");

        //TODO: need to check backslash for password
        // change password
        fprintf(fp[1], "%s", input);
        fflush(fp[1]);

        // parse 'password' output
        while (fgets(buf, sizeof(buf), fp[0]) != NULL)
        {
            HexLogInfo("login policy:console:%s", buf);

            if (strstr(buf, "all authentication tokens updated successfully") != NULL) {
                HexLogDebug("Changed %s password", user);
                result = EXIT_SUCCESS;
                break;
            }

            if (strstr(buf, "failure") != NULL || strstr(buf, "error") != NULL) {
                result = 1;
                break;
            }

            if ((pos = strstr(buf, "You must choose a longer password")) != NULL) {
                fprintf(stdout, "%s", pos);
                result = 2;
                break;
            }
            else if ((pos = strstr(buf, "You must wait longer to change your password")) != NULL) {
                fprintf(stdout, "%s", pos);
                result = 3;
                break;
            }
            else if ((pos = strstr(buf, "Password unchanged")) != NULL) {
                fprintf(stdout, "%s", pos);
                result = 4;
                break;
            }
            else if ((pos = strstr(buf, "Password has been already used. Choose another.")) != NULL) {
                fprintf(stdout, "%s", pos);
                result = 5;
                //need to let the child process exits
                int state = kill(pid, SIGTERM);
                if (state != 0) {
                    HexLogWarning("The change password process is not existent.");
                }
                break;
            }
            else if ((pos = strstr(buf, "BAD PASSWORD: ")) != NULL) {

                HexLogWarning("Password change failure: %s", errmsg);

                strncpy(errmsg, (const char *)pos, sizeof(errmsg) - 1);
                fprintf(stdout, "%s", errmsg);

                /* It is better to show precise error msg to consumer
                 * So consumer can know why the new password meet the password complexity
                 * But we can not just return the strings coz they need to be translated.
                 */
                const char *BAD_PWD_MSG[] = {
                        "new and old password are too similar",
                        "it is WAY too short",
                        "it is too short",
                        "it is too simplistic/systematic",
                        "it does not contain enough DIFFERENT characters",
                        "it is based on a dictionary word",
                        "it is based on your username"
                };

                const int size_of_BADPWDMGS = sizeof(BAD_PWD_MSG) / sizeof(char*);
                result = 6; // use default BAD PASSWORD msg that is not in one of the seven.
                for (int i = 0; i < size_of_BADPWDMGS; i++) {
                    if ((pos = strstr(buf, BAD_PWD_MSG[i])) != NULL) {
                        result = result + i + 1;
                        break;
                    }
                }
                break;
            }
        }

        fclose(fp[0]);
        fclose(fp[1]);

        waitpid(pid, &status, 0);
    }

    return result;
}

static void PasswordUsage(void)
{
    fprintf(stderr, "Usage: %s password <old password> <new password>\n", HexLogProgramName());
}

static int PasswordMain(int argc, char* argv[])
{
    // Check we've been called with correct number of arguments
    if (argc != 3) {
        HexLogError("Incorrect number of arguments specified to hex_config password");
        PasswordUsage();
        return 1;
    }

    char* oldpassword = argv[1];
    char* newpassword = argv[2];

    if (oldpassword == NULL || strlen(oldpassword) == 0 ||
        newpassword == NULL || strlen(newpassword) == 0) {
        HexLogError("Password must be specified to %s password", HexLogProgramName());
        return 1;
    }

    // leverage passwd to change root/admin password and it will validate chage state before change
    char input[128];
    snprintf(input, sizeof(input), "%s\n%s\n%s\n", oldpassword, newpassword, newpassword);
    int result = ChangeItsPassword(ADMIN_USER, input, newpassword);
    if (result == EXIT_SUCCESS) {
        HexLogInfo("Successfully updated %s password", ADMIN_USER);
    }
    else {
        HexLogError("Could not change %s password with the error code %d", ADMIN_USER, result);
        return result;
    }

    snprintf(input, sizeof(input), "%s\n%s\n", newpassword, newpassword);
    result = ChangeItsPassword("root", input, newpassword);
    if (result == EXIT_SUCCESS) {
        HexLogInfo("Successfully updated %s password", "root");
    }
    else {
        HexLogError("Could not change %s password with the error code %d", "root", result);
        return result;
    }

    HexSystemF(0, "rm -f /boot/grub2/user.cfg");
    result = HexSystemF(0, "(/usr/bin/sleep 1 && /usr/bin/echo %s && /usr/bin/sleep 1 && /usr/bin/echo %s) | /usr/bin/script -qf -c 'grub2-set-password -o /boot/grub2' >/dev/null 2>&1", newpassword, newpassword);
    HexSystemF(0, "rm -f /boot/grub2/typescript");
    if (result == EXIT_SUCCESS) {
        HexLogInfo("Successfully updated GRUB %s password", "root");
    }
    else {
        HexLogError("Could not change GRUB %s password with the error code %d", "root", result);
        return result;
    }

    HexZeroizeMemory(oldpassword, strlen(oldpassword));
    HexZeroizeMemory(newpassword, strlen(newpassword));

    return EXIT_SUCCESS;
}

static void CheckPasswordUsage(void)
{
    fprintf(stderr, "Usage: %s check_password <usernmae>\n", HexLogProgramName());
}

static int CheckPasswordMain(int argc, char* argv[])
{
    struct spwd* pwd;
    time_t elapsed;

    if (argc != 2) {
        HexLogError("Incorrect number of arguments specified to hex_config check_password");
        CheckPasswordUsage();
        return 1;
    }

    // argv[1]: username
    if((pwd = getspnam(argv[1])) == (struct spwd*)0) {
        HexLogError("Unknown user %s", argv[1]);
        return 1;
    }

    if (pwd->sp_lstchg == 0) {
        // new authtok required
        return EXIT_SUCCESS;
    }

    enum {
        ACCT_PASSWD_VALID = 0,
        ACCT_PASSWD_WARN,
        ACCT_PASSWD_EXPIRED
    };


    /* check all other possibilities
     * sp_lstchg : last password change
     * sp_max    : days before change required
     * sp_warn   : days warning for expiration
     * sp_inact  : days before account inactive
     */
    elapsed = (time(NULL) / SECS_PER_DAY) - pwd->sp_lstchg;
    int flag = ACCT_PASSWD_VALID;

    if ((elapsed > (pwd->sp_max + pwd->sp_inact)) && (pwd->sp_max != -1) && (pwd->sp_inact != -1))
        flag = ACCT_PASSWD_EXPIRED;
    else if ((elapsed > pwd->sp_max) && (pwd->sp_max != -1))
        flag = ACCT_PASSWD_EXPIRED;
    else if (elapsed > (pwd->sp_max - pwd->sp_warn))
        flag = ACCT_PASSWD_WARN;

    if (flag == ACCT_PASSWD_EXPIRED) {
        HexLogError("%s password expired", pwd->sp_namp);
        //TODO: HexLogEvent("password expired")
    }
    else if (flag == ACCT_PASSWD_WARN) {
        HexLogWarning("%s password will expire in %ld days", pwd->sp_namp, (pwd->sp_max - elapsed + 1));
        //TODO: HexLogEvent("password is going to expired")
    }

    return EXIT_SUCCESS;
}

static bool
MigrateMain(const char * prevVersion, const char * prevRootDir)
{
    HexUtilSystemF(0, 0, HEX_SDK " EtcShadowMigrate %s", prevRootDir);
    return true;
}

CONFIG_MODULE(password, 0, Parse, 0, 0, Commit);

CONFIG_COMMAND_WITH_SETTINGS(password, PasswordMain, PasswordUsage);
CONFIG_COMMAND(check_password, CheckPasswordMain, CheckPasswordUsage);

CONFIG_MIGRATE(password, MigrateMain);

