// HEX SDK

#define _GNU_SOURCE	// GNU program_invocation_short_name
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <hex/tuning.h>

int
main(int argc, char *argv[])
{
    // Read tuning parameters from file "settings"
    // Convert tuning parameter name into shell variable by replace periods with underscores and prepending "var-prefix"
    // If "name-prefix" is specified, process only those parameters that match, otherwise process all

    char *namePrefix = 0;
    if (argc == 4) {
        namePrefix = argv[3];
    } else if (argc != 3) {
        fprintf(stderr, "Usage: %s <settings> <var-prefix> [ <name-prefix> ]\n", program_invocation_short_name);
        return 1;
    }
    char *settings = argv[1];
    char *newPrefix = argv[2];

    int status = 0;
    FILE *fin = fopen(settings, "r");
    if (fin) {
        HexTuning_t tun = HexTuningAlloc(fin);
        if (tun) {
            const char *name, *value;
            while (1) {
                int result = HexTuningParseLine(tun, &name, &value);
                if (result == HEX_TUNING_EOF) break;
                if (result != HEX_TUNING_SUCCESS) {
                    fprintf(stderr, "Error: Could not parse file\n");
                    status = 1;
                    break;
                }
                const char *p;
                if (namePrefix == 0 || HexMatchPrefix(name, namePrefix, &p)) {
                    printf("%s", newPrefix);
                    // Replace all periods and hyphens in name with underscores
                    const char *q = name;
                    while (*q != '\0') {
                        size_t n = strcspn(q, ".-");
                        if (n == 0) {
                            printf("_");
                            q++;
                        } else {
                            printf("%.*s", (int)n, q);
                            q += n;
                        }
                    }
                    printf("='");
                    // Replace special characters in value with underscores
                    q = value;
                    while (*q != '\0') {
                        size_t n = strcspn(q, "\\\"'`$");
                        if (n == 0) {
                            printf("_");
                            q++;
                        } else {
                            printf("%.*s", (int)n, q);
                            q += n;
                        }
                    }
                    printf("'\n");
                }
            }
            HexTuningRelease(tun);
        } else {
            fprintf(stderr, "Error: Malloc failed\n");
            status = 1;
        }
        fclose(fin);
    } else {
        fprintf(stderr, "Error: Could not open file: %s\n", settings);
        status = 1;
    }

    return status;
}

