// HEX SDK

#ifndef CLI_PASSWORD_H
#define CLI_PASSWORD_H

#include <unistd.h> // close, unlink, ...
#include <errno.h>

#include <hex/cli_util.h>
#include <hex/auth.h>
#include <hex/process.h>
#include <hex/tempfile.h>

/**
 * All the user visible strings
 */
static const char* LABEL_OLD_PASSWORD_ENTRY = "Enter old password: ";
static const char* LABEL_NEW_PASSWORD_ENTRY = "Enter new password: ";
static const char* LABEL_CONFIRM_PASSWORD_ENTRY = "Confirm new password: ";
static const char* MSG_PASSWORD_MISMATCH = "New password and confirmation do not match";
static const char* MSG_PASSWORD_INCORRECT = "Old password is incorrect.";
static const char* MSG_PASSWORD_CHANGE_SUCCESS = "Password successfully changed.";
static const char* MSG_PASSWORD_CHANGE_FAILED = "Password could not be changed.";
static const char* MSG_AUTHN_INIT_FAILED = "Could not initialize authentication system.";

#define MIN_PASSWORD_LENGTH 6

// The configuration for a local-auth only authentication validator
static const char CONFIG_FILE_CONTENT [] =
    "[AUTH_SERVER local]\n"
    "AliasName = local\n"
    "local = true\n"
    "SERVER_TYPE = 0\n";

/**
 * A class to handle the password change functionality. It prompts the user to
 * supply a password, validates that using hex auth, then prompts the user for
 * the new password and to confirm the new password. The password change is
 * applied using hex_config password.
 *
 * This only works for the local 'admin' user at present.
 */
class PasswordChanger
{
public:
    PasswordChanger()
     : m_configFile("")
    {
        m_initialized = initialize();
    }

    ~PasswordChanger()
    {
        if (m_initialized) {
            HexAuthFini();
        }
    }

    /**
     * Run the password change operation. This prompts the user for the previous
     * password, the new password and a confirmation of the new password. If
     * everything checks out, it invokes hex_config password to perform the
     * password change operation.
     *
     * The return value indicates whether the password has been successfully
     * updated or not. This also prints an error indicating the success or
     * failure of the operation.
     */
    bool configure()
    {
        if (!m_initialized) {
            CliPrintf(MSG_AUTHN_INIT_FAILED);
            return false;
        }

        char rep = ' ';
        size_t len;
        std::string oldPassword;
        bool modified = false;
        CliReadPassword(LABEL_OLD_PASSWORD_ENTRY, oldPassword);
        if (validatePassword(oldPassword)) {
            std::string newPassword;
            std::string confirmPassword;
            CliReadPassword(LABEL_NEW_PASSWORD_ENTRY, newPassword);
            CliReadPassword(LABEL_CONFIRM_PASSWORD_ENTRY, confirmPassword);

            if (newPassword != confirmPassword) {
                CliPrintf(MSG_PASSWORD_MISMATCH);
            }
            else {
                if (setPassword(oldPassword, newPassword)) {
                    CliPrintf(MSG_PASSWORD_CHANGE_SUCCESS);
                    modified = true;
                } else {
                    CliPrintf(MSG_PASSWORD_CHANGE_FAILED);
                }
            }

            // Blank out the password strings
            len = newPassword.length();
            newPassword.replace(0, len, len, rep);
            len = confirmPassword.length();
            confirmPassword.replace(0, len, len, rep);
        }

        // Blank out the password strings
        len = oldPassword.length();
        oldPassword.replace(0, len, len, rep);

        return modified;
    }

private:

    bool m_initialized;
    std::string m_configFile;


    /**
     * Create a Hex Auth configuration file that only contains a local
     * authentication entry.
     *
     * Returns true on success, false otherwise.
     */
    bool createConfigFile(std::string &configFile)
    {
        // Have we successfully written the config file
        bool success = false;

        // Create the temporary file, abort if this doesn't work
        HexTempFile tmpfile;
        if (tmpfile.fd() < 0) {
            HexLogError("cli_password could not create temporary file: %d %s",
                    errno, strerror(errno));
            return false;
        }

        // Write the config to the file
        ssize_t bytesWritten = write(tmpfile.fd(), CONFIG_FILE_CONTENT,
                                     sizeof(CONFIG_FILE_CONTENT));

        // Check if it succeeded
        if (bytesWritten < 0) {
            HexLogError("cli_password could not write file contents to disk: %d %s",
                        errno, strerror(errno));
        }
        else if (bytesWritten != sizeof(CONFIG_FILE_CONTENT)) {
            HexLogError("cli_password couldn't write entire contents of file to disk. Only wrote %zd bytes",
                        bytesWritten);
        }
        else {
            success = true;
        }

        // Close the file descriptor
        tmpfile.close();

        if (success) {
            configFile = tmpfile.release();
        }

        return success;
    }

    bool initialize()
    {
        bool initialized = false;
        std::string configFile;

        // Create the config file
        if (!createConfigFile(configFile))  {
            return false;
        }

        // Use that to initialize hex_auth
        if (HexAuthInit(configFile.c_str()) == HEX_AUTH_SUCCESS) {
            initialized = true;
        }

        // Delete the config file
        if (unlink(configFile.c_str()) < 0) {
            HexLogWarning("cli_password couldn't delete file %s : %d %s",
                          configFile.c_str(), errno, strerror(errno));
        }

        return initialized;
    }

    /**
     * Check whether the password supplied is valid for the admin user.
     */
    bool validatePassword(const std::string oldPassword)
    {
        HexAuthCreds creds;

        creds.user = (char *)"admin";
        creds.pass = (char *)oldPassword.c_str();

        HexAuthUserInfo info;
        memset(&info, 0, sizeof(info));

        bool status=false;

        if (HexAuth(&creds, &info) == HEX_AUTH_SUCCESS) {
            status = true;
        }
        else {
            CliPrintf(MSG_PASSWORD_INCORRECT);
            status = false;
        }

        if (info.username) free(info.username);
        if (info.group) free(info.group);
        if (info.as) free(info.as);

        return status;
    }

    /**
     * Set the admin user's password to the new value
     */
    bool setPassword(const std::string oldPassword, const std::string newPassword)
    {
        int rc = HexSpawn(0, "/usr/sbin/hex_config", "password",
                          oldPassword.c_str(), newPassword.c_str(), (const char*)0);

        int exitCode = HexExitStatus(rc);
        if (exitCode == 0) {
            return true;
        }

        return false;
    }

};

#endif /* endif CLI_PASSWORD_H */

