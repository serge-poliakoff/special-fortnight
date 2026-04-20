#ifndef SEMANTICS
    #define SEMANTICS 1
    #include "tree.h"
    
    
    

    void analyse_semantics(Node* tree);

    //todo: for tests only, make static on prod.
    char* check_type(Node* Exp, Node* localVars, Node** localtypes);
    char* check_function_call(Node* funcNode, Node* localVars, Node** localtypes);
    void analyse_variables(Node* declVars, Node** typetable, int struct_flag);
    void analyse_func(Node* func);
    Node* built_func_tree(const char* name);
#endif