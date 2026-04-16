#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <vartable.h>

#define HASH_BACKET 343

// todo: create hash and hashtables for structs and hashtables for structs and variables respectively
// maybe do some fancy static declaration
// todo: make function to retreive a full vartable (it makes much more sence since compiler goes func by func)
VarTab globalTable = {"", NULL, 0 };

extern void addFunctionVars(char* func_name, VarNode* vars, int tab_size){
    if (strcmp(func_name, "global") != 0){
        fprintf(stderr, "Not implemented\n");
        exit(-1);
    }
    globalTable.key = func_name;
    globalTable.vars = vars;
    globalTable.size = tab_size;
    for (int i = 0; i < globalTable.size; i++){
        
        printf("%s: resb %lld, %s\n", vars[i].id, vars[i].size,
            vars[i].addr_type == STATIC ? "static" : "relative");
    }
}

extern VarTab getVarTable(char* func_name){
    if (strcmp(func_name, "global") != 0){
        fprintf(stderr, "Not implemented\n");
        exit(-1);
    }
    return globalTable;
}

extern VarNode* getVariable(char* func_name, char* id){
    if (strcmp(func_name, "global") != 0){
        fprintf(stderr, "Not implemented\n");
        exit(-1);
    }
    for (int i = 0 ; i < globalTable.size; i ++){
        if (strcmp(id, globalTable.vars[i].id)){
            return &globalTable.vars[i];
        }
    }
    fprintf(stderr, "Compile error: variable %s not found in %s space\n", id, func_name);
    exit(3);
}

extern void freeVarTables(){
    free(globalTable.vars);
}