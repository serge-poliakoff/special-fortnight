#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <vartable.h>

#define VARTAB_HASHTABLE_SIZE 101

/*
"func_name":
    var_table:      // array with known size
        {                                  
            vars:
                [ {"a", RELATIVE, -4, NULL, 4},         // int local var 
                {"b", RELATIVE, -8, {vars : [
                    {"w", REL, 0, NULL, 1},
                    {"h", REL, -1, NULL, 1}], size: 2},
                    2}  // struct with "w" and "h" char fields
                ],
            size: 2
        }
    structs:    // linked list
        [
            {
                struct_name: "rect",
                fields: {vars : [                     // same as in variable "b"
                    {"w", REL, 0, NULL, 1},
                    {"h", REL, -1, NULL, 1}], size: 2},
                size: 2
                next: NULL
            }
        ],
    next: "func_name_2" {if same hash}
*/


//todo:
//  - add semantic tests on repeating function declaration / structure declaration

// Hashtable node
typedef struct VarTabHashNode {
    char* key;
    struct VarTab* table;
    StructListNode* structs;
    struct VarTabHashNode* next;
} VarTabHashNode;



// Hashtable
typedef struct {
    VarTabHashNode** buckets;
    size_t size;
} VarTabHashTable;

/*
Feeing strategy:
    - all keys are dups and must be freed
    - all VarTab objects are allocated inside this module and must be freed also
    - all VarNode* pointers are allocated dynamically during semantic analysis and must be freed
    - all fields of VarNode are pointing to the programm tree / to structure typetables, so they must not be freed
    - all the structure typetables must be freed following same rules
*/


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
VarTabHashNode* vartab_hashtable_insert(VarTabHashTable* ht, const char* key) {
    unsigned int idx = vartab_hash(key);
    VarTabHashNode* node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            
            return node;
        }
        node = node->next;
    }
    node = malloc(sizeof(VarTabHashNode));
    node->key = strdup(key);
    node->table = NULL;
    node->structs = NULL;
    node->next = ht->buckets[idx];
    ht->buckets[idx] = node;
    return node;
}

static VarTabHashNode* vartab_hashtable_find(VarTabHashTable* ht, const char* key) {
    unsigned int idx = vartab_hash(key);
    VarTabHashNode* node = ht->buckets[idx];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node;
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
            VarTabHashNode* next = node -> next;

            StructListNode* cur_s = node -> structs;
            while (cur_s)
            {
                StructListNode* next = cur_s -> next;
                printf("Deallocation: struct %s\n", cur_s->struct_name);
                printf("%p", cur_s -> fields);
                printf("%p", cur_s -> fields->vars);
                printf("%s - %ld",cur_s->fields->vars[0].id, cur_s->fields->vars[0].size);
                free(cur_s->struct_name);
                // if structures are nested, those fields will be deallocated
                //  during deallocation for nested structures' declarations,
                //  so no worry here
                free(cur_s->fields->vars);
                free(cur_s->fields);
                free(cur_s);
                cur_s = next;
            }

            free(node -> key);
            free(node -> table -> vars);
            free(node -> table);
            //free all structures
            
            printf("Deallocation: structs proceeded\n");
            free(node);
            node = next;
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


extern void addFunctionVars(char* func_name, VarNode* vars, int tab_size){
    VarTab* tab = (VarTab*)malloc(sizeof(VarTab));
    if (tab == NULL){
        fprintf(stderr,"Critical error: Allocation fail\n");
        exit(-1);
    }
    tab->vars = vars;
    tab->size = tab_size;
    VarTabHashTable* fdict = funcs_var_dict();


    VarTabHashNode* node = vartab_hashtable_find(fdict, func_name);
    if (node == NULL){
        node = vartab_hashtable_insert(fdict, strdup(func_name));
    }else if (node->table != NULL){
        //evry func node has only one VarDecl, so if it is set already it means
        //  that function is declared twice
        fprintf(stderr, "Semantic error: function %s was already declared\n",
            func_name);
        exit(2);
    }
    node -> table = tab;
}

/// @brief add struct type data to the typetable
/// @param func_name name of function, in which structure is declared, or "global" if it is in the global scope. Do not duplicate string
/// @param struct_name name of structure type. Do not duplicate string
/// @param fields an array of structure's fields
/// @param tab_size size of "fields" table
extern void addStuctVars(char* func_name, char* struct_name, VarNode* fields, int tab_size){
    
    VarTab* tab = (VarTab*)malloc(sizeof(VarTab));
    if (tab == NULL){
        fprintf(stderr,"Critical error: Allocation fail\n");
        exit(-1);
    }
    tab->vars = fields;
    tab->size = tab_size;

    //calculate size of a structure
    size_t struct_size = 0;
    for (int i = 0; i < tab_size; i++){
        struct_size += fields[i].size;
    }
    //create struct node
    StructListNode* snode = (StructListNode*)malloc(sizeof(StructListNode));
    if (!snode) {
        fprintf(stderr, "Critical error: Allocation fail for StructListNode\n");
        exit(-1);
    }
    snode->struct_name = strdup(struct_name);
    snode->fields = tab;
    snode->size = struct_size;
    snode->next = NULL;

    //finding function node
    VarTabHashTable* fdict = funcs_var_dict();
    VarTabHashNode* node = vartab_hashtable_find(fdict, func_name);
    if (node == NULL){
        node = vartab_hashtable_insert(fdict, strdup(func_name));
    }

    // Debug: print current struct list before insertion
    printf("[DEBUG] Current struct list for %s before insertion: ", func_name);
    StructListNode* dbg_cur = node->structs;
    while (dbg_cur) {
        printf("%s ", dbg_cur->struct_name);
        dbg_cur = dbg_cur->next;
    }
    printf("\n");

    // Check for duplicate struct name
    StructListNode* cur = node->structs;
    while (cur) {
        if (strcmp(cur->struct_name, struct_name) == 0) {
            fprintf(stderr, "Semantic error: struct type %s is declared twice\n", struct_name);
            exit(2);
        }
        cur = cur->next;
    }

    // Insert at head
    snode->next = node->structs;
    node->structs = snode;

    // Debug: print struct list after insertion
    printf("[DEBUG] Struct list for %s after insertion: ", func_name);
    dbg_cur = node->structs;
    while (dbg_cur) {
        printf("%s ", dbg_cur->struct_name);
        dbg_cur = dbg_cur->next;
    }
    printf("\n");
}

extern VarTab getVarTable(char* func_name){
    VarTabHashNode* func_Node = vartab_hashtable_find(funcs_var_dict(), func_name);
    if (func_Node == NULL){
        fprintf(stderr, "Table for %s function was never registered", func_name);
        exit(-1);
    }
    return *(func_Node->table);
}


extern StructListNode* getStructType(char* func_name, char* struct_name){
    VarTabHashNode* func_Node = vartab_hashtable_find(funcs_var_dict(), func_name);
    if (func_Node == NULL){
        return NULL;
    }
    printf("Searching for struct %s in %s table\n",
        struct_name, func_name);
    StructListNode* cur = func_Node -> structs;
    while (cur != NULL)
    {
        printf("%s\t", cur->struct_name);
        if (strcmp(cur->struct_name, struct_name) == 0){
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

extern void freeVarTables(){
    printf("Deallocating dictionnaries...\n");
    vartab_hashtable_free(funcs_var_dict());
}