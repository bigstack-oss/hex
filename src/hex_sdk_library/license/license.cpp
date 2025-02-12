// HEX SDK

#include <iostream>

#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <cmath>

#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/pem.h>

#include <sys/sysinfo.h>

#include <hex/lock.h>
#include <hex/log.h>
#include <hex/process.h>
#include <hex/process_util.h>
#include <hex/string_util.h>
#include <hex/crypto.h>
#include <hex/license.h>
#include <hex/filesystem.h>
#include <hex/tuning.h>

#include "license_key.h"

bool
RSAVerifySignature(const std::string& strLicenseDataFile, const std::string& strLicenseSigFile)
{
    bool bRet(true);

    BIO* bio = BIO_new_mem_buf((void*)PUBLIC_KEY, -1);
    EVP_PKEY* pKey = PEM_read_bio_PUBKEY(bio, NULL, NULL, NULL);
    if (NULL == pKey) {
        bRet = false;
    }
    BIO_free(bio);

    if (!bRet)
        return bRet;

    std::ifstream l(strLicenseDataFile);
    std::stringstream lic;
    lic << l.rdbuf();
    lic.seekg(0, std::ios::end);

    std::ifstream s(strLicenseSigFile);
    std::stringstream sig;
    sig << s.rdbuf();
    sig.seekg(0, std::ios::end);

    EVP_MD_CTX* ctx = EVP_MD_CTX_create();

    if (EVP_DigestVerifyInit(ctx, NULL, EVP_sha256(), NULL, pKey) <= 0) {
        return false;
    }

    if (EVP_DigestVerifyUpdate(ctx, (const unsigned char*)lic.str().c_str(), lic.tellg()) <= 0) {
        return false;
    }

    int AuthStatus = EVP_DigestVerifyFinal(ctx, (const unsigned char*)sig.str().c_str(), sig.tellg());

    EVP_MD_CTX_free(ctx);

    if (AuthStatus != 1) {
        return false;
    }

    return true;
}

int
HexLicenseCheck(const std::string& app, std::string *type, std::string *serial, const std::string& filename)
{
    *type = "none";
    *serial = "";

    std::string suffix = "";
    if (app.length() > 0 && app != "def") {
        suffix = "-" + app;
    }

    std::string licenseData = "/etc/update/license" + suffix + ".dat";
    std::string licenseSig = "/etc/update/license" + suffix + ".sig";
    if (filename.length()) {
        licenseData = filename + ".dat";
        licenseSig = filename + ".sig";
    }

    if (access(licenseData.c_str(), F_OK) != 0 ||
        access(licenseSig.c_str(), F_OK) != 0) {
        return LICENSE_NOEXIST;
    }
    else {
        if (RSAVerifySignature(licenseData, licenseSig)) {

            FILE *fin = fopen(licenseData.c_str(), "r");
            if (!fin) {
                HexLogError("Could not read license file");
                return LICENSE_BADSYS;
            }

            HexTuning_t tun = HexTuningAlloc(fin);
            if (!tun) {
                HexLogError("Could not allocate memory");
                return LICENSE_BADSYS;
            }

            int ret;
            const char *name, *value;
            std::string issue_hardware;
            std::string expiry_date;

            while ((ret = HexTuningParseLine(tun, &name, &value)) != HEX_TUNING_EOF) {
                if (ret != HEX_TUNING_SUCCESS) {
                    // Malformed, exceeded buffer, etc.
                    HexLogError("Could not read license values");
                    return LICENSE_BADSYS;
                }

                // display tuning params as text
                if (strcmp(name, "license.type") == 0) {
                    *type = value;
                }
                else if (strcmp(name, "issue.hardware") == 0) {
                    issue_hardware = value;
                }
                else if (strcmp(name, "expiry.date") == 0) {
                    expiry_date = value;
                }
            }

            HexTuningRelease(tun);
            fclose(fin);

            std::vector<std::string> serials = hex_string_util::split(issue_hardware, ',');

            char s[256];
            memset(s, 0, sizeof(s));
            fin = fopen("/sys/class/dmi/id/product_serial", "r");
            fgets(s, sizeof(s), fin);
            s[strcspn(s, "\r")] = '\0';
            s[strcspn(s, "\n")] = '\0';
            fclose(fin);

            char * s_copy = (char *) malloc(strlen(s) + 1);
            char * s_trim = s_copy;
            int s_idx = 0;
            strcpy(s_copy, s);
            while (s[s_idx] != '\0') {
                if(!isspace(s[s_idx]))
                    *s_copy++ = s[s_idx];

                s_idx++;
            }
            *s_copy = '\0';
            *serial = std::string(s_trim);

            bool found = false;
            for (auto& s : serials) {
                if (s.compare(*serial) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found && issue_hardware != "*") {
                return LICENSE_BADHW;
            }

            // check if the license has been expired
            struct tm tmExpiry;
            memset(&tmExpiry, 0, sizeof(struct tm));
            strptime(expiry_date.c_str(), "%Y-%m-%d %H:%M:%S UTC", &tmExpiry);
            time_t tExpiry = mktime(&tmExpiry);

            time_t now = time(NULL); // get current UTC time
            double period = difftime(tExpiry, now);
            if (period < 0) {
                return LICENSE_EXPIRED;
            }

            return (std::ceil)(period / 86400);

        }
        else {
            return LICENSE_BADSIG;
        }
    }

    return LICENSE_OK;
}

