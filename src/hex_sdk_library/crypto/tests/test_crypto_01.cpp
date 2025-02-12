// HEX SDK

#include <hex/log.h>
#include <hex/crypto.h>
#include <hex/test.h>

int main() {
    HexLogInit("test_crypto_01", 1 /*logToStdErr*/);

    std::string pass;

#if 0
    // run sha256sum /usr/bin/openssl to adjust the test
    HEX_TEST(HexCryptoFileHashCheck("/usr/bin/openssl", "051f6ecc6ed6dc0648007dfee3fb088590fa714215eca0021aae1bc267d8096c"));
    HEX_TEST(!HexCryptoFileHashCheck("/usr/bin/openssl", "051f6ecc6ed6dc0648007dfee3fb088590fa714215eca0021aae1bc267d8096"));
#endif

    HexCryptoGetPassphrase(16, "keyA", "keyB", &pass);
    HEX_TEST(pass == "NKZu7Xiy7+3Zn9FX");

    HexCryptoGetPassphrase(32, "keyAC", "keyB", &pass);
    HEX_TEST(pass == "RCjzSOtD150Gt4a6ukCPwysbCyM=");

    HexCryptoGetPassphrase(64, "keyA", "keyBD", &pass);
    HEX_TEST(pass == "mPxYwrdBHZ0f+J9ZgFkWTKIyM3g=");

    HexCryptoGetPassphrase(128, "keyAC", "keyBD", &pass);
    HEX_TEST(pass == "HV3uQexy/GJVVWIGiPYI1tJaKK4=");

    HexCryptoGetPassphrase(512, "keyAC", "keyBD", &pass);
    HEX_TEST(pass == "HV3uQexy/GJVVWIGiPYI1tJaKK4=");

    HexCryptoGetBytesInBase64(16, "keyA", "keyB", &pass);
    HEX_TEST(pass == "AzA8bXaCWG3iuXm//IZJWQ==");

    HexCryptoGetBytesInBase64(32, "keyAC", "keyB", &pass);
    HEX_TEST(pass == "KPt8LvpoedXjBTmgLr0WscgZxSaXVoYAWEDHGOu91hc=");

    HexCryptoGetBytesInBase64(64, "keyA", "keyBD", &pass);
    HEX_TEST(pass == "gsB4yGjGL+7AWElPRismVlk4+AySzIn4kMPzLupa2F9s0mNwezDO+RUCILKLMD0X8W+deRh0A5mXTXo6Iyo9uQ==");

    HexCryptoGetBytesInBase64(128, "keyAC", "keyBD", &pass);
    HEX_TEST(pass == "43WnVcbTDSpXZiI+BMZQINIMTSfXY692bML6bEhMGNl/vCG6D0DkngIGwMkdUAER2vphnykxketghY7k+vd5VA==");

    HexCryptoGetBytesInBase64(512, "keyAC", "keyBD", &pass);
    HEX_TEST(pass == "43WnVcbTDSpXZiI+BMZQINIMTSfXY692bML6bEhMGNl/vCG6D0DkngIGwMkdUAER2vphnykxketghY7k+vd5VA==");

    /* MAX: RSA_size(rsa) - 11 */
    /* e.g. 8192 / 8 - 11 = 1013 bytes */
    std::string text = "VtkM6wXvA29QKmvveaifTWwSSfueNryPfvvgH92ptrxBa6NNdB2UqNCgu6Mcr9xXkbZHh3BSJVme5xtweiQXGPy43CQyRbwpEcZ5uzDeJMzQkubf48xYiwg55p33VgnFVjqXN3KA3JPmgd66vVgZqi4MVuavtLzBvTLHi6p86F5daXzKzRCdyYX2fzfdeNxUnG8CdcRdBGMDXegLrqeBvdrcEWhhmWNQbnD7UtvqyR95uA8cr6GEeHkPiLU4x3DYbU5pW6uAPKdga5p5Z5fGqxEmSiqWqJ4KFw9Bz9iELEjK2pjAehGvi5jNVWUJ5bhAcaTGvdqhVJWVpR6WDHZYMxGYzcqe3Z6FpniPLGLq7UML7YhtL3hPuuqc8mFMNDyiy9GSqitc5FRpwxt4dnQwqmyM5zchPEuhupBfHxfZXPyGaCa8Xh4r4Qa3dharr48aE4H2wtFErMGNn9wYfQGGL2iEanHbw7Tq87jdBEyYhJeQyKYVjzbjceGKcKQZB9PaL9xEC668KzgMMriCS7teX8YdyuQ9fjZSqrHMB6NEC2Er86y8JaN68k5uccP4688buCyS6Q9VAFJpxMUxUKCWiiqLuDXLf2HSBNjSvtyjMrvxH2pi5Y2FFPVY36H8GizBjawT6b2DjjLZrfpEJPYXVTUJZwifHXmQfPaCGDJSR8WC47gnPaMmMyFyLvbJ9KyL2DdbgYW3LGEh6aw9m9JCha354q4TkJ9Z5krJ3cKVSGa4ZHncpbLdcBUKE7bZ3wfRL48AZw4wv5aS7BCeiwuYcfS2UTSFJZgAa2SyqjWDmLEDMCZhgN7Q7yPHPiWbnnbwrRNPnuwa6Sy6cyGRqVwatr2MRwGMrjDhU9qgYhNn46DKk5EAyXcHBkzBF74CSy8f9ZFPitvZVfkPhDmxaeqCGPpKWjHw8FrbRjk92XgpSfGKqQmhgTzF2kHddYgjaBeTpSFJFqpruEx3VJ39fn2Suy6bhe3tCNnnwg5r9NY5qaD27hWPz5znw";

    std::string plain, cipher;
    HexCryptoEncryptData(text, &cipher);
    HexCryptoDecryptData(cipher, &plain);
    HEX_TEST(plain == text);

    std::string sign, unsign;
    HexCryptoSignData(text, &sign);
    HexCryptoUnSignData(sign, &unsign);
    HEX_TEST(unsign == text);

#if 0
    GenRsaPassphrase(&pass);
    HEX_TEST(pass == "3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT");

    std::string b64key;
    GenBase64KeyPart("3mE5ydqfYfLmTEGaCBX4JdkjdcQg9yTSwNqygcLeti4Fc48kmiW8nXqvV7PhHhT", &b64key);
    printf("%s\n", b64key.c_str());
#endif
    return HexTestResult;
}
