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
}; // "Store as" switch

// private functions
void processLayer(yaml_parser_t *parser, GNode *data);
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
    freeTree(cfg, NULL);
    cfg = NULL;
}

int
ReadYml(const char *policyFile, GNode *cfg)
{
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
    processLayer(&parser, cfg);

    // Cleanup
    yaml_parser_delete(&parser);
    fclose(source);

    return 0;
}

int
WriteYml(const char *policyFile, GNode *cfg)
{
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
    if (path == NULL)
        return cfg;

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
        if (matched)
            node = g_node_first_child(matched);
        else
            break;

        key = strtok_r(NULL, ".", &context);
    }

    free(ypath);

    return matched;
}

const char*
FindYmlValue(GNode *cfg, const char *path)
{
    GNode* value = g_node_first_child(FindYmlNode(cfg, path));
    if (value)
        return (char*)value->data;
    else
        return NULL;
}

const char*
FindYmlValueF(GNode *cfg, const char *fmt, ...)
{
    char *path = NULL;
    const char* value = NULL;

    va_list ap;
    va_start(ap, fmt);
    if (vasprintf(&path, fmt, ap) < 0)
        return NULL;
    va_end(ap);

    printf("%s\n", path);
    value = FindYmlValue(cfg, path);
    free(path);

    return value;
}

int
UpdateYmlValue(GNode *cfg, const char *path, const char *value)
{
    GNode* node = g_node_first_child(FindYmlNode(cfg, path));
    if (!node)
        return -1;

    g_free(node->data);
    gchar *new = g_strdup((gchar *)value);
    node->data = new;

    return 0;
}

int
AddYmlNode(GNode *cfg, const char *path, const char *key, const char *value)
{
    GNode* node = FindYmlNode(cfg, path);
    if (!node)
        return -1;

    gchar *gkey = g_strdup((gchar *)key);
    gchar *gvalue = g_strdup((gchar *)value);

    GNode *keyNode = g_node_append(node, g_node_new(gkey));
    g_node_append_data(keyNode, gvalue);

    return 0;
}

int
AddYmlKey(GNode *cfg, const char *path, const char *key)
{
    GNode* node = FindYmlNode(cfg, path);
    if (!node)
        return -1;

    gchar *gkey = g_strdup((gchar *)key);
    g_node_append(node, g_node_new(gkey));

    return 0;
}

// free all its children and itself
int
DeleteYmlNode(GNode *cfg, const char *path)
{
    GNode* node = FindYmlNode(cfg, path);
    if (!node)
        return -1;

    FiniYml(node);

    return 0;
}

// free all its children
int
DeleteYmlChildren(GNode *cfg, const char *path)
{
    GNode* node = FindYmlNode(cfg, path);
    if (!node)
        return -1;

    g_node_children_foreach(node, G_TRAVERSE_ALL, freeTree, NULL);

    return 0;
}


size_t
SizeOfYmlSeq(GNode *cfg, const char *path)
{
    GNode* node = FindYmlNode(cfg, path);
    if (node)
        return (unsigned int)g_node_n_children(node);
    else
        return 0;
}

void
TraverseYml(GNode *cfg, TraverselFunc func, gpointer data)
{
    // Start traversing
    g_node_traverse(cfg, G_PRE_ORDER, G_TRAVERSE_ALL, -1, func, data);
}

/* shows an example of using yml util APIs */
void
DumpYml(const char *policyFile)
{
    GNode *cfg = InitYml(policyFile);

    if (ReadYml(policyFile, cfg))
        TraverseYml(cfg, dumpNode, NULL);

    FiniYml(cfg);
}

void
DumpYmlNode(GNode *node)
{
    TraverseYml(node, dumpNode, NULL);
}

void
processLayer(yaml_parser_t *parser, GNode *data)
{
    GNode *last_leaf = data;
    yaml_event_t event;
    int storage = VAR; // mapping cannot start with VAL definition w/o VAR key
    int seqidx = 0;

    while (1) {
        yaml_parser_parse(parser, &event);

        // Parse value either as a new leaf in the mapping
        // or as a leaf value (one of them, in case it's a sequence)
        if (event.type == YAML_SCALAR_EVENT) {
            gchar *value = g_strdup((gchar *)event.data.scalar.value);
            if (storage) // val
                g_node_append_data(last_leaf, value);
            else // key
                last_leaf = g_node_append(data, g_node_new(value));

            // Flip VAR/VAL switch for the next event
            storage ^= VAL;
        }

        // Sequence - the following scalars will be appended to the seq index node
        else if (event.type == YAML_SEQUENCE_START_EVENT) {
            seqidx = 1;
            storage = SEQ;
        }
        else if (event.type == YAML_SEQUENCE_END_EVENT)
            storage = VAR;

        // depth += 1
        else if (event.type == YAML_MAPPING_START_EVENT) {
            if (storage & SEQ) {
                gchar *value = g_strdup_printf("%d", seqidx++);
                GNode *idxNode = g_node_append(last_leaf, g_node_new(value));
                processLayer(parser, idxNode);
            }
            else
                processLayer(parser, last_leaf);

            // Flip VAR/VAL, w/o touching SEQ
            storage ^= VAL;
        }

        // depth -= 1
        else if (event.type == YAML_MAPPING_END_EVENT || event.type == YAML_STREAM_END_EVENT)
            break;

        yaml_event_delete(&event);
    }
}

gboolean writeNode(GNode *node, gpointer data)
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
        while (--i)
            fprintf(dest, "  ");

        if (atoi(node->data) > 0) {
            //seq node
            fprintf(dest, "- \n");
        }
        else {
            fprintf(dest, "%s: ", (char*)node->data);

            // if its child is not leaf, the node is start of a node/map/seq.
            // Adding a newline
            if(!G_NODE_IS_LEAF(g_node_first_child(node)))
                fprintf(dest, "\n");
        }
    }
    else {
        // leaf node
        fwrite(node->data, 1, strlen(node->data), dest);
        fprintf(dest, "\n");
    }

    return(FALSE);
}

gboolean dumpNode(GNode *node, gpointer data)
{
    int i = g_node_depth(node);
    while (--i)
        printf("  ");

    printf("%s\n", (char *)node->data);
    return(FALSE);
}

gboolean freeNode(GNode *node, gpointer data)
{
    g_free(node->data);
    return(FALSE);
}

void freeTree(GNode *node, gpointer data)
{
    g_node_traverse(node, G_PRE_ORDER, G_TRAVERSE_ALL, -1, freeNode, NULL);
    g_node_destroy(node);
}

