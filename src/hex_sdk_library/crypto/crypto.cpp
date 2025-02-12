// HEX SDK

#include <iostream>
#include <iomanip>
#include <sstream>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include <algorithm>

#include <hex/crypto.h>
#include <hex/log.h>
#include <hex/zeroize.h>

static const std::string strPublicKeyFile("/etc/update/public.pem");
static const std::string strPrivateKeyFile("/etc/update/private.pem");

static const char ZERO_STRING[] = "00000000000000000000000000000000000000000000000000"
                                  "00000000000000000000000000000000000000000000000000";

static bool ssl_init = false;

bool
HexCryptoFileHashCheck(const std::string& binary, const std::string& checksum)
{
    FILE *file = fopen(binary.c_str(), "rb");
    if (!file)
        return false;

    const int bufSize = 32768;
    int bytesRead = 0;

    unsigned char hash[EVP_MAX_MD_SIZE];
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);

    unsigned char *buffer = (unsigned char*)malloc(bufSize);
    if (!buffer)
        return false;

    while((bytesRead = fread(buffer, 1, bufSize, file))) {
        EVP_DigestUpdate(mdctx, buffer, bytesRead);
    }


    unsigned int hashLength;
    EVP_DigestFinal_ex(mdctx, hash, &hashLength);
    EVP_MD_CTX_free(mdctx);

    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < hashLength; ++i) {
      ss << std::setw(2) << static_cast<unsigned int>(hash[i]);
    }

    fclose(file);
    free(buffer);

    if (ss.str() != checksum)
        return false;

    return true;
}

bool
HexCryptoBase64(const std::string& strInput, std::string* strOutput)
{
    bool bRet(true);
    strOutput->clear();

    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());

    bio = BIO_push(b64, bio);
    BIO_write(bio, strInput.data(), (int)strInput.length());
    (void)BIO_flush(bio);

    char* bptr = NULL;
    long blen = BIO_get_mem_data(bio, &bptr);
    strOutput->append(bptr, blen);
    BIO_free_all(bio);
    std::string::iterator it;
    it = std::remove(strOutput->begin(), strOutput->end(), '\n');
    strOutput->erase(it, strOutput->end());

    return bRet;
}

bool
HexCryptoUnBase64(const std::string& strInput, std::string* strOutput)
{
    bool bRet(true);
    strOutput->clear();

    BIO* bio = BIO_new(BIO_s_mem());
    for (size_t j = 0 ; j < strInput.length(); j += 64) {
        size_t bytes = strInput.length() - j;
        if (bytes > 64)
            bytes = 64;
        BIO_write( bio, &strInput.at(j), (int)bytes);
        BIO_write( bio, "\n", 1);  // had problems unless \n terminated
    }

    (void)BIO_flush(bio);
    BIO_set_mem_eof_return(bio, 0);

    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    size_t t =  BIO_ctrl_pending(bio);
    unsigned char buf[32768] = { 0 };
    int i = BIO_read(bio, (void*) buf, (int)t);
    strOutput->append((char*)buf, i);
    BIO_free_all(bio);
    return bRet;
}

bool
HexCryptoGetPassphrase(const int len,
                       const std::string& strKeyA,
                       const std::string& strKeyB,
                       std::string* strPassphrase)
{
    *strPassphrase = ZERO_STRING;
    strPassphrase->clear();

    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    unsigned char value[EVP_MAX_MD_SIZE];

    EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL);
    memset(&value[0], 0, EVP_MAX_MD_SIZE);

    if (EVP_DigestUpdate(mdctx, strKeyA.c_str(), strKeyA.length()) == 0 ||
        EVP_DigestUpdate(mdctx, strKeyB.c_str(), strKeyB.length()) == 0) {
        HexLogError("Could not update message digest");
        return false;
    }

    unsigned int hashLength;
    if (EVP_DigestFinal_ex(mdctx, value, &hashLength) == 0) {
        HexLogError("Could not finalize message digest");
        return false;
    }

    std::string strValue(value, value + hashLength);
    std::string strBase64;
    HexCryptoBase64(strValue, &strBase64);

    *strPassphrase = strBase64.substr(0, len);

    return true;
}

bool
HexCryptoGetBytesInBase64(const int len,
                          const std::string& strKeyA,
                          const std::string& strKeyB,
                          std::string* strBytesInBase64)
{
    *strBytesInBase64 = ZERO_STRING;
    strBytesInBase64->clear();

    unsigned char value[EVP_MAX_MD_SIZE];
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    EVP_DigestInit_ex(mdctx, EVP_sha512(), NULL);
    memset(&value[0], 0, EVP_MAX_MD_SIZE);

    if (EVP_DigestUpdate(mdctx, strKeyA.c_str(), strKeyA.length()) == 0 ||
        EVP_DigestUpdate(mdctx, strKeyB.c_str(), strKeyB.length()) == 0) {
        HexLogError("Could not update message digest");
        return false;
    }

    unsigned int hashLength;
    if (EVP_DigestFinal_ex(mdctx, value, &hashLength) == 0) {
        HexLogError("Could not finalize message digest");
        return false;
    }

    std::string strValue(value, value + (len > (int)hashLength ? EVP_MAX_MD_SIZE : len));
    std::string strBase64;
    HexCryptoBase64(strValue, &strBase64);

    *strBytesInBase64 = strBase64;

    return true;
}

static void
ScrambleKey(const std::string& asciiKey, const std::string& base64Key, std::string* passphrase)
{
    *passphrase = ZERO_STRING;
    passphrase->clear();

    std::string unbase64Key;
    HexCryptoUnBase64(base64Key, &unbase64Key);

    std::string binaryKey = ZERO_STRING;
    binaryKey.clear();
    for (int i = 0 ; i < 63 ; ++i)
        *passphrase += asciiKey[i] ^ unbase64Key[i];
}

void
GenRsaPassphrase(std::string* passphrase)
{
    // 63 bytes longs
    std::string asciiKey = "(bunWCP@8)r,nuY[)wg-NzmnP@S(4$jX7LTUg,N}:vD6B77h4S:,wUT)%pi#eV)";
    std::string base64Key("Gw8wWy4nISZhTz5BOjAeOmo1PxkEHgYENCMCTw1dPgtAAiUsAE8CGE4fcHAhAw8DWTptFBkNJV9zRzlLLT59");
    ScrambleKey(asciiKey, base64Key, passphrase);
    HexZeroizeMemory(asciiKey);
    HexZeroizeMemory(base64Key);
}

#if 0
static void
GenBase64KeyPart(const std::string& passphrase, std::string* base64key)
{
    // 63 bytes longs
    std::string asciiKey = "(bunWCP@8)r,nuY[)wg-NzmnP@S(4$jX7LTUg,N}:vD6B77h4S:,wUT)%pi#eV)";
    std::string unbase64key = ZERO_STRING;
    unbase64key.clear();

    for (int i = 0 ; i < 63 ; ++i)
        unbase64key += passphrase[i] ^ asciiKey[i];

    HexCryptoBase64(unbase64key, base64key);
}
#endif

static bool
PublicRSA(const std::string& strInput, const std::string& strPublicKeyFile, std::string* strOutput, bool encrypted)
{
    bool bRet(true);
    strOutput->clear();

    // Test encryption with the public key.
    BIO* pIn = BIO_new_file(strPublicKeyFile.c_str(), "r");
    if (NULL == pIn) {
        HexLogError("Could not open the public key file '%s'", strPublicKeyFile.c_str());
        bRet = false;
    }
    EVP_PKEY* pKey = PEM_read_bio_PUBKEY(pIn, NULL, NULL, NULL);
    if (NULL == pKey) {
        HexLogError("Could not process the public key file");
        unsigned long err = ERR_get_error();
        HexLogError("Error Number:%lX, Error lib:'%s', reason:'%s'",
                    err,
                    ERR_lib_error_string(err),
                    ERR_reason_error_string(err));
        bRet = false;
    }
    BIO_free(pIn);

    if (!bRet)
        return bRet;

    size_t cbOutputLen = 0;
    unsigned char* pOutputBuf = NULL;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pKey, NULL);

    int iResult;
    if (encrypted) {
        EVP_PKEY_encrypt_init(ctx);
        iResult = EVP_PKEY_encrypt(ctx, 0, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
        pOutputBuf = (unsigned char*)malloc(cbOutputLen);
        iResult = EVP_PKEY_encrypt(ctx, pOutputBuf, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
    }
    else {
        EVP_PKEY_verify_recover_init(ctx);
        iResult = EVP_PKEY_verify_recover(ctx, 0, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
        pOutputBuf = (unsigned char*)malloc(cbOutputLen);
        iResult = EVP_PKEY_verify_recover(ctx, pOutputBuf, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
    }

    if (iResult == 1) {
        strOutput->append((char*)pOutputBuf, cbOutputLen);
    }
    else {
        HexLogError("PublicRSA: Could not %s the data", encrypted ? "encrypt" : "decrypt");
        unsigned long err = ERR_get_error();
        HexLogError("Error Number:%lX, Error lib:'%s', reason:'%s'",
                    err,
                    ERR_lib_error_string(err),
                    ERR_reason_error_string(err));
        bRet = false;
    }

    if (pOutputBuf) {
        HexZeroizeMemory(pOutputBuf, cbOutputLen);
        free(pOutputBuf);
    }

    EVP_PKEY_free(pKey);
    EVP_PKEY_CTX_free(ctx);

    return bRet;
}

static bool
PrivateRSA(const std::string& strInput, const std::string& strPrivateKeyFile,
           const std::string& strPassphrase, std::string* strOutput, bool encrypted)
{
    bool bRet( true);
    strOutput->clear();

    BIO* pIn = BIO_new_file(strPrivateKeyFile.c_str(), "r");
    if (NULL == pIn) {
        HexLogError("Could not open the private key file '%s'", strPrivateKeyFile.c_str());
        bRet = false;
    }

    EVP_PKEY* pKey = PEM_read_bio_PrivateKey(pIn, NULL, NULL, (char*)strPassphrase.c_str());
    if(NULL == pKey) {
        HexLogError("Could not process the private key file");
        unsigned long err = ERR_get_error();
        HexLogError("Error Number:%lX, Error lib:'%s', reason:'%s'",
                    err,
                    ERR_lib_error_string(err),
                    ERR_reason_error_string(err));
        bRet = false;
    }
    BIO_free(pIn);

    if (!bRet)
        return bRet;

    size_t cbOutputLen = 0;
    unsigned char* pOutputBuf = NULL;

    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(pKey, NULL);

    int iResult;
    if (encrypted) {
        EVP_PKEY_sign_init(ctx);
        iResult = EVP_PKEY_sign(ctx, 0, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
        pOutputBuf = (unsigned char*)malloc(cbOutputLen);
        iResult = EVP_PKEY_sign(ctx, pOutputBuf, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
    }
    else {
        EVP_PKEY_decrypt_init(ctx);
        iResult = EVP_PKEY_decrypt(ctx, 0, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
        pOutputBuf = (unsigned char*)malloc(cbOutputLen);
        iResult = EVP_PKEY_decrypt(ctx, pOutputBuf, &cbOutputLen, (const unsigned char *)strInput.c_str(), strInput.size());
    }

    if (iResult == 1) {
        strOutput->append((char*)pOutputBuf, cbOutputLen);
    }
    else {
        HexLogError("PrivateRSA: Could not %s the data", encrypted ? "encrypt" : "decrypt");
        unsigned long err = ERR_get_error();
        HexLogError("Error Number:%lX, Error lib:'%s', reason:'%s'",
                    err,
                    ERR_lib_error_string(err),
                    ERR_reason_error_string(err));
        bRet = false;
    }

    if(pOutputBuf) {
        HexZeroizeMemory(pOutputBuf, cbOutputLen);
        free(pOutputBuf);
    }

    EVP_PKEY_free(pKey);
    EVP_PKEY_CTX_free(ctx);

    return bRet;
}

int
HexCryptoEncryptData(const std::string &clearInput, std::string *base64EncryptedOutput)
{
    if (!ssl_init) {
        OpenSSL_add_all_algorithms();
        ssl_init = true;
    }

    std::string strEncryptedTemp;
    bool bResult = PublicRSA(clearInput, strPublicKeyFile, &strEncryptedTemp, true);

    if (bResult) {
        // Now base64 encode the data.
        std::string strB64EncryptedOutput;
        bResult = HexCryptoBase64(strEncryptedTemp, &strB64EncryptedOutput);

        if (bResult) {
            *base64EncryptedOutput = strB64EncryptedOutput;
            HexZeroizeMemory(strB64EncryptedOutput);
        }
    }

    HexZeroizeMemory(strEncryptedTemp);

    return bResult ? 0 : -1;
}

int
HexCryptoDecryptData(const std::string &base64EncryptedInput, std::string *clearOutput)
{
    if (!ssl_init) {
        OpenSSL_add_all_algorithms();
        ssl_init = true;
    }

    std::string strEncrypted;
    bool bResult = HexCryptoUnBase64(base64EncryptedInput, &strEncrypted);

    if (bResult) {
        // Need the passprhase to access the private key
        std::string strPassphrase;
        GenRsaPassphrase(&strPassphrase);

        // Now decrypt the data
        std::string strClearOutput;
        bResult = PrivateRSA(strEncrypted, strPrivateKeyFile, strPassphrase, &strClearOutput, false);

        if (bResult) {
            *clearOutput = strClearOutput;
            HexZeroizeMemory(strClearOutput);
        }

        // Must zero and clear passphrase, encrypted password,  and clear text password
        HexZeroizeMemory(strEncrypted);
        HexZeroizeMemory(strPassphrase);
        strPassphrase.clear();
    }

    return bResult ? 0 : -1;
}

int
HexCryptoSignData(const std::string &clearInput, std::string *base64SignedOutput)
{
    if (!ssl_init) {
        OpenSSL_add_all_algorithms();
        ssl_init = true;
    }

    // Need the passprhase to access the private key
    std::string strPassphrase;
    GenRsaPassphrase(&strPassphrase);

    // Sign the data
    std::string strSignedOutput;
    bool bResult = PrivateRSA(clearInput, strPrivateKeyFile, strPassphrase, &strSignedOutput, true);

    if (bResult) {
        // Now base64 encode the data.
        std::string strBase64SignedOutput;
        bResult = HexCryptoBase64(strSignedOutput, &strBase64SignedOutput);

        if (bResult) {
            *base64SignedOutput = strBase64SignedOutput;
            HexZeroizeMemory(strBase64SignedOutput);
        }
    }

    // Must zero and clear passphrase, signed output
    HexZeroizeMemory(strSignedOutput);
    HexZeroizeMemory(strPassphrase);
    strPassphrase.clear();

    return bResult ? 0 : -1;
}

int
HexCryptoUnSignData(const std::string &base64SignedOutput, std::string *clearOutput)
{
    if (!ssl_init) {
        OpenSSL_add_all_algorithms();
        ssl_init = true;
    }

    std::string strSigned, strClear;
    bool bResult = HexCryptoUnBase64(base64SignedOutput, &strSigned);

    if (bResult) {
        bResult = PublicRSA(strSigned, strPublicKeyFile, &strClear, false);

        if (bResult) {
            *clearOutput = strClear;
            HexZeroizeMemory(strClear);
        }
    }

    HexZeroizeMemory(strSigned);

    return bResult ? 0 : -1;
}

