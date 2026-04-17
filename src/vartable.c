#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <vartable.h>

#define VARTAB_HASHTABLE_SIZE 101

// Hashtable node
typedef struct VarTabHashNode {
    char* key;
    VarTab* table;
    struct VarTabHashNode* next;
} VarTabHashNode;

// Hashtable
typedef struct {
    VarTabHashNode** buckets;
    size_t size;
} VarTabHashTable;

unsigned int vartab_hash(const char* key) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % VARTAB_HASHTABLE_SIZE;
}

VarTabHashTable* vartab_hashtable_create() {
    VarTabHashTable* ht = malloc(sizeof(VarTabHashTable));
    if (ht == NULL){
        fprintf(stderr, "Allocation error\n");
        exit(-1);
    }
    ht->size = VARTAB_HASHTABLE_SIZE;
    ht->buckets = calloc(ht->size, sizeof(VarTabHashNode*));
    if (ht->buckets == NULL){
        fprintf(stderr, "Allocation error\n");
        exit(-1);
    }
    return ht;
}

/// @attention ALWAYS DUPLICATE KEY BEFORE INSERTING
void vartab_hashtable_insert(VarTabHashTable* ht, const char* key, VarTab* table) {
    unsigned int idx = vartab_hash(key);
    VarTabHashNode* node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            node->table = table;
            return;
        }
        node = node->next;
    }
    node = malloc(sizeof(VarTabHashNode));
    node->key = strdup(key);
    node->table = table;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
}

VarTab* vartab_hashtable_find(VarTabHashTable* ht, const char* key) {
    unsigned int idx = vartab_hash(key);
    VarTabHashNode* node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->table;
        }
        node = node->next;
    }
    return NULL;
}

void vartab_hashtable_free(VarTabHashTable* ht) {
    if (ht == NULL) return;
    for (size_t i = 0; i < ht->size; ++i) {
        VarTabHashNode* node = ht->buckets[i];
        while (node) {
            free(node -> key);
            free(node -> table -> vars);
            free(node -> table);
            free(node);
            node = node->next;
        }
    }
    free(ht->buckets);
    free(ht);
}

static VarTabHashTable* funcs_var_dict(){
    static VarTabHashTable *dict= NULL;
    if (dict == NULL){
        dict = vartab_hashtable_create();
        printf("Func-vartable dictionary created\n");
    }
    return dict;
}

static VarTabHashTable* struct_fields_dict(){
    static VarTabHashTable *dict= NULL;
    if (dict == NULL){
        dict = vartab_hashtable_create();
    }
    return dict;
}


extern void addFunctionVars(char* func_name, VarNode* vars, int tab_size){
    VarTab* tab = (VarTab*)malloc(sizeof(VarTab));
    if (tab == NULL){
        fprintf(stderr,"Critic error: Allocation fail\n");
        exit(-1);
    }
    tab->vars = vars;
    tab->size = tab_size;
    VarTabHashTable* fdict = funcs_var_dict();
    vartab_hashtable_insert(fdict, strdup(func_name), tab);
}

extern void addStuctVars(char* struct_name, VarNode* fields, int tab_size){
    VarTab* tab = (VarTab*)malloc(sizeof(VarTab));
    if (tab == NULL){
        fprintf(stderr,"Critic error: Allocation fail\n");
        exit(-1);
    }
    tab->vars = fields;
    tab->size = tab_size;
    VarTabHashTable* sdict = struct_fields_dict();
    vartab_hashtable_insert(sdict, strdup(struct_name), tab);
}

extern VarTab getVarTable(char* func_name){
    VarTab* result = vartab_hashtable_find(funcs_var_dict(), func_name);
    if (result == NULL){
        fprintf(stderr, "Table for %s function was never registered", func_name);
        exit(-1);
    }
    return *result;
}


extern void freeVarTables(){
    printf("Deallocating dictionnaries...");
    vartab_hashtable_free(funcs_var_dict());
    vartab_hashtable_free(struct_fields_dict());
}