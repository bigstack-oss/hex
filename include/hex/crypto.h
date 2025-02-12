// HEX SDK

#ifndef HEX_CRYPTO_H
#define HEX_CRYPTO_H

// Process extended API requires C++
#ifdef __cplusplus

#include <string>

bool HexCryptoFileHashCheck(const std::string& binary, const std::string& checksum);
bool HexCryptoBase64(const std::string& strInput, std::string* strOutput);

bool
HexCryptoGetPassphrase(const int len, const std::string& strKeyA,
                       const std::string& strKeyB, std::string* strPassphrase);
bool
HexCryptoGetBytesInBase64(const int len,
                          const std::string& strKeyA,
                          const std::string& strKeyB,
                          std::string* strBase64);

void GenRsaPassphrase(std::string* passphrase);

int HexCryptoEncryptData(const std::string &clearInput, std::string *base64EncryptedOutput);
int HexCryptoDecryptData(const std::string &base64EncryptedInput, std::string *clearOutput);

int HexCryptoSignData(const std::string &clearInput, std::string *base64SignedOutput);
int HexCryptoUnSignData(const std::string &base64SignedOutput, std::string *clearOutput);

#if 0
void GenRsaPassphrase(std::string* passphrase);
void GenBase64KeyPart(const std::string& passphrase, std::string* base64key);
#endif

#endif // __cplusplus

#endif /* endif HEX_CRYPTO_H */


