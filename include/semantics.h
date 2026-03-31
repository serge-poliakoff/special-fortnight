#ifndef SEMANTICS
    #define SEMANTICS 1
    #include "tree.h"
    
    void analyse_semantics(Node* tree);

    //for tests only, make static on prod.
    char* check_type(Node* Exp, Node* localVars, Node** localtypes);

#endif