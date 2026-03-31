#include <stdio.h>
#include <stdlib.h>

#include "tree.h"
#define MAX_TYPES 100

Node* glob_vars;    //pointer to program's global VarDecl
Node* glob_types[MAX_TYPES];    //global custom types (structs)

extern void analyse_semantics(Node* tree){
    glob_vars = NULL;
    for(int i = 0; i < MAX_TYPES; i++) glob_types[i] = NULL;


    return;
}