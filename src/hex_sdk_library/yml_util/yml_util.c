// HEX SDK

#define _GNU_SOURCE // vasprintf
#include <stdio.h>
#include <stdarg.h> // va_xxx

#include <stdlib.h> // atoi
#include <hex/log.h>

#include <hex/yml_util.h>

enum storage_flags {
    VAR = 0,
    VAL,
    SEQ
}; // "store as" switch

// private functions
int processLayer(yaml_parser_t *parser, GNode *data);
gboolean writeNode(GNode *node, gpointer data);
gboolean dumpNode(GNode *node, gpointer data);
gboolean freeNode(GNode *node, gpointer data);
void freeTree(GNode *node, gpointer data);

GNode*
InitYml(const char *rootName)
{
    return g_node_new(g_strdup((gchar *)rootName));
}

void
FiniYml(GNode *cfg)
{
    if (cfg == NULL) {
        return;
    }

    freeTree(cfg, NULL);
    cfg = NULL;
}

int
ReadYml(const char *policyFile, GNode *cfg)
{
    if (cfg == NULL) {
        return -1;
    }

    yaml_parser_t parser;

    FILE *source = fopen(policyFile, "rb");
    if (!source) {
        HexLogError("Could not open policy file %s", policyFile);
        return -1;
    }

    // Initialize parser
    if(!yaml_parser_initialize(&parser)) {
        HexLogError("Failed to initialize policy yaml parser");
        return -1;
    }

    yaml_parser_set_input_file(&parser, source);

    // Recursive parsing
    int ret = processLayer(&parser, cfg);
    if (ret < 0) {
        if (parser.error != YAML_NO_ERROR) {
            HexLogError("Failed to parse policy file %s: %s%s%s at line %zu, column %zu",
                        policyFile, parser.problem ? parser.problem : "syntax error",
                        parser.context ? " " : "", parser.context ? parser.context : "",
                        parser.problem_mark.line + 1, parser.problem_mark.column + 1);
        } else {
            HexLogError("Failed to parse policy file %s: unexpected end of yaml events", policyFile);
        }
    }

    // Cleanup
    yaml_parser_delete(&parser);
    fclose(source);

    return ret;
}

int
WriteYml(const char *policyFile, GNode *cfg)
{
    if (cfg == NULL) {
        return -1;
    }

    FILE *dest = fopen(policyFile, "wb");
    if (!dest) {
        HexLogError("Could not open policy file %s", policyFile);
        return -1;
    }

    g_node_traverse(cfg, G_PRE_ORDER, G_TRAVERSE_ALL, -1, writeNode, dest);

    fclose(dest);
    return 0;
}

GNode*
FindYmlNode(GNode *cfg, const char *path)
{
    if (cfg == NULL) {
        return NULL;
    }
    if (path == NULL) {
        return cfg;
    }

    char* context = NULL;

    // convert const char* to char*
    // strtok_r required local variable
    char* ypath = strdup(path);
    char* key = strtok_r(ypath, ".", &context);

    // skip root node which is doc name
    GNode *node = g_node_first_child(cfg);
    GNode *matched = NULL;

    while (key) {
        matched = NULL;

        while (node) {
            if (strcmp((const char*)node->data, key) == 0) {
                matched = node;
                break;
            }

            // try next sibling for matched key
            node = g_node_next_sibling(node);
        }

        // go deeper if matched
        if (matched) {
            node = g_node_first_child(matched);
        } else {
            break;
        }

        key = strtok_r(NULL, ".", &context);
    }

    free(ypath);
    return matched;
}

const char*
FindYmlValue(GNode *cfg, const char *path)
{
    if (cfg == NULL) {
        return NULL;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (node == NULL) {
        return NULL;
    }

    GNode* value = g_node_first_child(node);
    if (value) {
        return (char*)value->data;
    } else {
        return NULL;
    }
}

const char*
FindYmlValueF(GNode *cfg, const char *fmt, ...)
{
    if (cfg == NULL) {
        return NULL;
    }

    char *path = NULL;
    const char* value = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0) {
        return NULL;
    }
    va_end(ap);

    printf("%s\n", path);
    value = FindYmlValue(cfg, path);
    free(path);

    return value;
}

int
UpdateYmlValue(GNode *cfg, const char *path, const char *value)
{
    if (cfg == NULL) {
        return -1;
    }

    GNode* parent = FindYmlNode(cfg, path);
    if (parent == NULL) {
        return -1;
    }
    GNode* node = g_node_first_child(parent);
    if (node == NULL) {
        return -1;
    }

    g_free(node->data);
    gchar *new = g_strdup((gchar *)value);
    node->data = new;

    return 0;
}

int
AddYmlNode(GNode *cfg, const char *path, const char *key, const char *value)
{
    if (cfg == NULL) {
        return -1;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (!node) {
        return -1;
    }

    gchar *gkey = g_strdup((gchar *)key);
    gchar *gvalue = g_strdup((gchar *)value);

    GNode *keyNode = g_node_append(node, g_node_new(gkey));
    g_node_append_data(keyNode, gvalue);

    return 0;
}

int
AddYmlKey(GNode *cfg, const char *path, const char *key)
{
    if (cfg == NULL) {
        return -1;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (!node) {
        return -1;
    }

    gchar *gkey = g_strdup((gchar *)key);
    g_node_append(node, g_node_new(gkey));

    return 0;
}

// free all its children and itself
int
DeleteYmlNode(GNode *cfg, const char *path)
{
    if (cfg == NULL) {
        return -1;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (!node) {
        return -1;
    }

    FiniYml(node);
    node = NULL;
    return 0;
}

// free all its children
int
DeleteYmlChildren(GNode *cfg, const char *path)
{
    if (cfg == NULL) {
        return -1;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (!node) {
        return -1;
    }

    g_node_children_foreach(node, G_TRAVERSE_ALL, freeTree, NULL);
    return 0;
}


size_t
SizeOfYmlSeq(GNode *cfg, const char *path)
{
    if (cfg == NULL) {
        return 0;
    }

    GNode* node = FindYmlNode(cfg, path);
    if (node) {
        return (unsigned int)g_node_n_children(node);
    } else {
        return 0;
    }
}

void
TraverseYml(GNode *cfg, TraverselFunc func, gpointer data)
{
    if (cfg == NULL) {
        return;
    }

    // Start traversing
    g_node_traverse(cfg, G_PRE_ORDER, G_TRAVERSE_ALL, -1, func, data);
}

/* shows an example of using yml util APIs */
void
DumpYml(const char *policyFile)
{
    GNode *cfg = InitYml(policyFile);

    if (ReadYml(policyFile, cfg) == 0) {
        TraverseYml(cfg, dumpNode, NULL);
    }

    FiniYml(cfg);
    cfg = NULL;
}

void
DumpYmlNode(GNode *node)
{
    if (node == NULL) {
        return;
    }

    TraverseYml(node, dumpNode, NULL);
}

// return 0 on success, -1 on a yaml parse error (details are left in parser)
int
processLayer(yaml_parser_t *parser, GNode *data)
{
    GNode *last_leaf = data;
    yaml_event_t event;
    int storage = VAR; // mapping cannot start with VAL definition w/o VAR key
    int seqidx = 0;

    while (1) {
        // once libyaml has failed, every later call yields YAML_NO_EVENT
        // and never the end event, so stop on either
        if (!yaml_parser_parse(parser, &event)) {
            return -1;
        }

        if (event.type == YAML_NO_EVENT) {
            yaml_event_delete(&event);
            return -1;
        }

        // Parse value either as a new leaf in the mapping
        // or as a leaf value (one of them, in case it's a sequence)
        if (event.type == YAML_SCALAR_EVENT) {
            gchar *value = g_strdup((gchar *)event.data.scalar.value);
            if (storage) {
                // val
                g_node_append_data(last_leaf, value);
            } else {
                // key
                last_leaf = g_node_append(data, g_node_new(value));
            }

            // Flip VAR/VAL switch for the next event
            storage ^= VAL;
        } else if (event.type == YAML_SEQUENCE_START_EVENT) {
            // Sequence - the following scalars will be appended to the seq index node
            seqidx = 1;
            storage = SEQ;
        } else if (event.type == YAML_SEQUENCE_END_EVENT) {
            storage = VAR;
        } else if (event.type == YAML_MAPPING_START_EVENT) {
            // depth += 1
            int ret;
            if (storage & SEQ) {
                gchar *value = g_strdup_printf("%d", seqidx++);
                GNode *idxNode = g_node_append(last_leaf, g_node_new(value));
                ret = processLayer(parser, idxNode);
            } else {
                ret = processLayer(parser, last_leaf);
            }

            if (ret < 0) {
                yaml_event_delete(&event);
                return -1;
            }

            // Flip VAR/VAL, w/o touching SEQ
            storage ^= VAL;
        } else if (event.type == YAML_MAPPING_END_EVENT || event.type == YAML_STREAM_END_EVENT) {
            // depth -= 1
            yaml_event_delete(&event);
            break;
        }

        yaml_event_delete(&event);
    }

    return 0;
}

// is value exactly one complete quoted scalar ("..." or '...')?
// some callers quote a value themselves (e.g. the alert_resp match expression),
// so it is written as is, like before
static gboolean
isQuotedScalar(const char *value, size_t len)
{
    char q = value[0];
    if (len < 2 || (q != '"' && q != '\'')) {
        return FALSE;
    }

    for (size_t i = 1; i < len; i++) {
        if (q == '"' && value[i] == '\\') {
            i++; // escaped character
        } else if (q == '\'' && value[i] == '\'' && value[i + 1] == '\'') {
            i++; // '' is an escaped '
        } else if (value[i] == q) {
            return i == len - 1;
        }
    }

    return FALSE;
}

// how a leaf scalar has to be written so that ReadYml() gets the same string back
static int
scalarStyle(const char *value)
{
    size_t len = strlen(value);

    // keep the empty placeholder ("key: ") and caller-quoted values as they are
    if (len == 0 || isQuotedScalar(value, len)) {
        return YAML_PLAIN_SCALAR_STYLE;
    }

    // line breaks and other control characters only survive with escapes
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)value[i];
        if ((c < 0x20 && c != '\t') || c == 0x7f) {
            return YAML_DOUBLE_QUOTED_SCALAR_STYLE;
        }
    }

    // a plain scalar loses leading and trailing blanks
    if (value[0] == ' ' || value[0] == '\t' || value[len - 1] == ' ' || value[len - 1] == '\t') {
        return YAML_SINGLE_QUOTED_SCALAR_STYLE;
    }

    // a plain scalar cannot start with an indicator
    if (strchr("#&*!|>'\"%@`,[]{}", value[0])) {
        return YAML_SINGLE_QUOTED_SCALAR_STYLE;
    }

    // '-', '?' and ':' are indicators only when followed by a blank (e.g. -1 stays plain)
    if (strchr("-?:", value[0]) && (value[1] == '\0' || value[1] == ' ' || value[1] == '\t')) {
        return YAML_SINGLE_QUOTED_SCALAR_STYLE;
    }

    // ": " starts a mapping value and " #" a comment
    if (value[len - 1] == ':' || strstr(value, ": ") || strstr(value, ":\t") ||
        strstr(value, " #") || strstr(value, "\t#")) {
        return YAML_SINGLE_QUOTED_SCALAR_STYLE;
    }

    return YAML_PLAIN_SCALAR_STYLE;
}

static void
writeScalar(FILE *dest, const char *value)
{
    int style = scalarStyle(value);

    if (style == YAML_SINGLE_QUOTED_SCALAR_STYLE) {
        fputc('\'', dest);
        for (const char *p = value; *p; p++) {
            if (*p == '\'') {
                fputc('\'', dest);
            }
            fputc(*p, dest);
        }
        fputc('\'', dest);
    } else if (style == YAML_DOUBLE_QUOTED_SCALAR_STYLE) {
        fputc('"', dest);
        for (const unsigned char *p = (const unsigned char *)value; *p; p++) {
            if (*p == '"' || *p == '\\') {
                fprintf(dest, "\\%c", *p);
            } else if (*p == '\n') {
                fprintf(dest, "\\n");
            } else if (*p == '\r') {
                fprintf(dest, "\\r");
            } else if (*p == '\t') {
                fprintf(dest, "\\t");
            } else if (*p < 0x20 || *p == 0x7f) {
                fprintf(dest, "\\x%02x", *p);
            } else {
                fputc(*p, dest);
            }
        }
        fputc('"', dest);
    } else {
        fwrite(value, 1, strlen(value), dest);
    }
}

gboolean
writeNode(GNode *node, gpointer data)
{
    FILE *dest = (FILE *)data;
    int i = g_node_depth(node) - 1;

    // write header
    if (i == 0) {
        fprintf(dest, "---\n");
        fprintf(dest, "# %s\n", (char*)node->data);
        return(FALSE);
    }

    if(!G_NODE_IS_LEAF(node)) {
        // non leaf node
        while (--i) {
            fprintf(dest, "  ");
        }

        if (atoi(node->data) > 0) {
            //seq node
            fprintf(dest, "- \n");
        } else {
            fprintf(dest, "%s: ", (char*)node->data);

            // if its child is not leaf, the node is start of a node/map/seq.
            // Adding a newline
            if(!G_NODE_IS_LEAF(g_node_first_child(node))) {
                fprintf(dest, "\n");
            }
        }
    } else {
        // leaf node
        writeScalar(dest, (const char *)node->data);
        fprintf(dest, "\n");
    }

    return(FALSE);
}

gboolean
dumpNode(GNode *node, gpointer data)
{
    int i = g_node_depth(node);
    while (--i) {
        printf("  ");
    }

    printf("%s\n", (char *)node->data);
    return(FALSE);
}

gboolean
freeNode(GNode *node, gpointer data)
{
    g_free(node->data);
    return(FALSE);
}

void
freeTree(GNode *node, gpointer data)
{
    g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, freeNode, NULL);
    g_node_destroy(node);
}

