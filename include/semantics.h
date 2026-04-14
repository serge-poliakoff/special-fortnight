#ifndef SEMANTICS
    #define SEMANTICS 1
    #include "tree.h"
    
    
    typedef struct var_node {
        char* id, *addr;
        /// @brief pointer to a list of fields of structure with their relative addresses. NULL for int or char. Do not free when cleaning vartables
        struct var_node** fields; 
        size_t size;
    } VarNode, *TypeNode;

    void analyse_semantics(Node* tree);

    //for tests only, make static on prod.
    char* check_type(Node* Exp, Node* localVars, Node** localtypes);
    char* check_function_call(Node* funcNode, Node* localVars, Node** localtypes);
    void analyse_variables(Node* declVars, Node** typetable);
    void analyse_func(Node* func);
#endif