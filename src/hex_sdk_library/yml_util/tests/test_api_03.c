// HEX SDK

#include <string.h>
#include <unistd.h>

#include <hex/test.h>

#include <hex/yml_util.h>

static const char *roundTrip[] = {
    "a: b",
    "x # y",
    "#general",
    "it's",
    " lead",
    "trail ",
    "-dash",
    "- dash",
    "ends:",
    "fe80::",
    "*/5 * * * *",
    "!tag",
    "[1, 2]",
    "{a}",
    "'quoted' x",
    "\"a\" == \"b\"",
    "a: 'b' # c",
    "line1\nline2",
    "tab\there",
    "back\\slash \"q\"\r",
};

// values that must still be written exactly as before (unquoted)
static const char *plain[][2] = {
    { "bool", "true" },
    { "int", "100" },
    { "neg", "-1" },
    { "float", "1.0" },
    { "empty", "" },
    { "host", "example.com" },
    { "ipv6", "::1" },
    { "url", "https://hooks.slack.com/services/x" },
    { "apos", "it's" },
    { "hash", "a#b" },
    { "paren", "(name() == 'system')" },
};

// a value that is already one quoted scalar is written as is (the caller quoted it,
// e.g. the alert_resp match expression); { written, read back }
static const char *preQuoted[][2] = {
    { "\"\\\"severity\\\" == 'W' OR \\\"severity\\\" == 'C'\"", "\"severity\" == 'W' OR \"severity\" == 'C'" },
    { "'it''s'", "it's" },
};

static char*
slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return NULL;
    }

    static char buf[8192];
    size_t n = fread(buf, 1, sizeof(buf) - 1, f);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

int main(int argc, char **argv)
{
    GNode *cfg = NULL;
    char key[32];
    char line[128];
    size_t i;

    // CASE1: a malformed file fails instead of spinning
    cfg = InitYml(argv[1]);
    HEX_TEST(ReadYml(argv[1], cfg) == -1);
    FiniYml(cfg);

    // CASE2: a malformed mapping inside a sequence fails too
    cfg = InitYml(argv[2]);
    HEX_TEST(ReadYml(argv[2], cfg) == -1);
    FiniYml(cfg);

    // CASE3: WriteYml() -> ReadYml() gives the same strings back
    cfg = InitYml("quote/quote1_0.yml");
    HEX_TEST(AddYmlNode(cfg, NULL, "name", "quote") == 0);
    for (i = 0; i < sizeof(roundTrip) / sizeof(roundTrip[0]); i++) {
        snprintf(key, sizeof(key), "v%zu", i);
        HEX_TEST(AddYmlNode(cfg, NULL, key, roundTrip[i]) == 0);
    }
    for (i = 0; i < sizeof(plain) / sizeof(plain[0]); i++) {
        HEX_TEST(AddYmlNode(cfg, NULL, plain[i][0], plain[i][1]) == 0);
    }

    for (i = 0; i < sizeof(preQuoted) / sizeof(preQuoted[0]); i++) {
        snprintf(key, sizeof(key), "q%zu", i);
        HEX_TEST(AddYmlNode(cfg, NULL, key, preQuoted[i][0]) == 0);
    }

    // a sequence of mappings, as the notification policy writes it
    HEX_TEST(AddYmlKey(cfg, NULL, "slacks") == 0);
    HEX_TEST(AddYmlKey(cfg, "slacks", "1") == 0);
    HEX_TEST(AddYmlNode(cfg, "slacks.1", "channel", "#general") == 0);
    HEX_TEST(AddYmlNode(cfg, "slacks.1", "note", "a: b") == 0);

    HEX_TEST(WriteYml("quote1_0.yml", cfg) == 0);
    FiniYml(cfg);

    cfg = InitYml("quote1_0.yml");
    HEX_TEST_FATAL(ReadYml("quote1_0.yml", cfg) == 0);
    for (i = 0; i < sizeof(roundTrip) / sizeof(roundTrip[0]); i++) {
        snprintf(key, sizeof(key), "v%zu", i);
        const char *v = FindYmlValue(cfg, key);
        HEX_TEST_STR(v && strcmp(v, roundTrip[i]) == 0, roundTrip[i]);
    }
    for (i = 0; i < sizeof(plain) / sizeof(plain[0]); i++) {
        const char *v = FindYmlValue(cfg, plain[i][0]);
        HEX_TEST_STR(v && strcmp(v, plain[i][1]) == 0, plain[i][1]);
    }
    for (i = 0; i < sizeof(preQuoted) / sizeof(preQuoted[0]); i++) {
        snprintf(key, sizeof(key), "q%zu", i);
        const char *v = FindYmlValue(cfg, key);
        HEX_TEST_STR(v && strcmp(v, preQuoted[i][1]) == 0, preQuoted[i][1]);
    }
    const char *v = FindYmlValue(cfg, "slacks.1.channel");
    HEX_TEST(v && strcmp(v, "#general") == 0);
    v = FindYmlValue(cfg, "slacks.1.note");
    HEX_TEST(v && strcmp(v, "a: b") == 0);
    FiniYml(cfg);

    // CASE4: values that read back fine (or were quoted by the caller) are written as before
    const char *out = slurp("quote1_0.yml");
    HEX_TEST_FATAL(out != NULL);
    for (i = 0; i < sizeof(plain) / sizeof(plain[0]); i++) {
        snprintf(line, sizeof(line), "\n%s: %s\n", plain[i][0], plain[i][1]);
        HEX_TEST_STR(strstr(out, line) != NULL, line);
    }
    HEX_TEST(strstr(out, "\nv0: 'a: b'\n") != NULL);
    HEX_TEST(strstr(out, "\nv3: it's\n") != NULL);
    HEX_TEST(strstr(out, "\nv14: '''quoted'' x'\n") != NULL);
    for (i = 0; i < sizeof(preQuoted) / sizeof(preQuoted[0]); i++) {
        snprintf(line, sizeof(line), "\nq%zu: %s\n", i, preQuoted[i][0]);
        HEX_TEST_STR(strstr(out, line) != NULL, line);
    }
    HEX_TEST(strstr(out, "channel: '#general'\n") != NULL);

    return HexTestResult;
}
