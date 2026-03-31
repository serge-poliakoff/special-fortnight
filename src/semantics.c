#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <tree.h>
#define MAX_TYPES 100

Node* glob_vars;    //pointer to program's global VarDecl
Node* glob_types[MAX_TYPES];    //global custom types (structs)
Node* functs;   //pointer to DeclFoncts node of tree

/// @brief analyses VarDecl node of programm or function and collects all structure type
/// declarations into a given typetable
/// @param declVars pointer to VarDecl node
/// @param typetable pointer to a preinitiallized type table of MAX_TYPES size (supposed initialized to all nulls)
static void analyse_variables(Node* declVars, Node** typetable){

}

/// @brief use to find a variable/field identity
/// @param name name of variable/field to find
/// @param place DeclVars or struct id node (childs are declarators)
/// @return name of type of the variable or NULL if not found
static char* lookup_var_type_between(char* name, Node* place){
    Node* cur = place -> firstChild;    //first declarator
    while (cur) {
        if (cur->label.type == TP) {
            Node* decl = cur->firstChild;
            while (decl) {
                if (strcmp(decl->label.value.id, name) == 0) {
                    // type is in cur->label.value.id
                    return cur->label.value.id;
                }
                decl = decl->nextSibling;
            }
        }
        cur = cur->nextSibling;
    }

    return NULL;
}

/// @brief checks for a type in a given array of types
/// @param type name of type to find
/// @param types array of type nodes
/// @return pointer to a node of the type or NULL
static Node* lookup_type_between(char* type, Node** types){
    for(int i = 0; types[i] != NULL; i++)
        if (strcmp(types[i]->label.value.id, type) == 0)
            return types[i];
    return NULL;
}

// Helper: lookup variable type in localVars or global vars
static char* lookup_var_type(Node* ident, Node* localVars, Node** localtypes) {
    // ident: Node* with label.type == ID, label.value.id is the name
    char* name = ident->label.value.id;
    
    char *type = lookup_var_type_between(name, localVars);
    type = type ? type : lookup_var_type_between(name, glob_vars);
    if (type == NULL){
        fprintf(stderr, "Semantic error: undeclared variable %s\n", name);
        //uncomment on prod - exit(2)\n
        return NULL;
    }

    Node* field = ident->firstChild;
    while (field)
    {
        //will run only if ident is a call to structure field, so type is a structure type
        Node* typeNode = lookup_type_between(type, localtypes);
        typeNode = typeNode ? typeNode : lookup_type_between(type, glob_types);
        if (typeNode == NULL){
            fprintf(stderr, "Semantic error: undeclared structure type %s\n", type);
            //uncomment on prod - exit(2)\n
            return NULL;
        }
        char* type1 = lookup_var_type_between(field->label.value.id, typeNode);
        if (type == NULL){
            fprintf(stderr, "Semantic error: field %s can't be found on %s\n", name, type);
            //uncomment on prod - exit(2);
            return NULL;
        }
        type = type1;
        field = field -> firstChild;
    }
    

    return type;
}

// Helper: check function call arguments (stub)
static char* check_function_call(Node* funcNode, Node* arguments, Node* localVars, Node** localtypes) {
    // TODO: implement function lookup and argument type checking
    // For now, just return "int" as a placeholder
    return "int";
}

/// @brief helper function for expression type analysing
/// @param typeName name of a type
/// @return 1 if type is int or char, 0 otherwise 
static int int_or_char(char* typeName){
    return ! (strcmp(typeName, "int") && strcmp(typeName, "char"));
}


/// @brief recursively checks the expression for semantic errors and determines its type
/// @param Exp pointer to expression node to analyse
/// @param localVars pointer to DeclVars of a current function
/// @param localtypes a null-terminated-array of locally declared structure types (must be non-null)
/// @return name of type of the expression
extern char* check_type(Node* Exp, Node* localVars, Node** localtypes){
    if (!Exp) return NULL;
    
    if (Exp->label.type == INT) return "int";
    
    if (Exp->label.type == CHAR) return "char";

    // IdExpr: function call / variable or field access
    if (Exp->label.type == ID) {
        Node* child = Exp->firstChild;
        // If function call: has Arguments child
        if (child != NULL){
            if (child->label.type == KEYWORD && child->label.value.label == Arguments) {
                // Function call
                return check_function_call(Exp, child, localVars, localtypes);
            }
        }
        // Otherwise, variable lookup
        return lookup_var_type(Exp, localVars, localtypes);
    }

    // Operators: arithmetic, order, equality (all but boolean operatora)
    if (Exp->label.type == OP) {
        char* type1Op = check_type(Exp->firstChild, localVars, localtypes);
        char* type2Op = check_type(Exp->firstChild->nextSibling, localVars, localtypes);
        if (type2Op == NULL){
            //unary - or +
            if (!int_or_char(type1Op)){
                fprintf(stderr, "Operator %s cannot be used with argument of type %s\n",
                    Exp->label.value.id, type1Op);
                //uncomment on prod - exit(2)\n
                return NULL;
            }
            return "int";
        }else{
            //operator with to operands
            if(!int_or_char(type1Op) || !int_or_char(type2Op)){
                fprintf(stderr, "Operator %s cannot be used with arguments of type %s and %s\n",
                    Exp->label.value.id, type1Op, type2Op);
                //uncomment on prod - exit(2)\n
                return NULL;
            }
            return "int";
        }
    }
    // AND, OR, NOT operators
    if (Exp->label.type == KEYWORD) {
        // we don't need to make assumptions about the type of
        // underlying expression(s), because any value apart zero will be taken as 1
        // however we need to check that typing in child(ren) is correct
        check_type(Exp->firstChild, localVars, localtypes);
        check_type(Exp->firstChild->nextSibling, localVars, localtypes);
        return "int";
    }
    
    /* Seems not to be present in the final tree
    Parenthesized expression: just propagate
    if (Exp->firstChild && !Exp->firstChild->nextSibling) {
        return check_type(Exp->firstChild, localVars, localtypes);
    }*/

    // if still don't return
    fprintf(stderr, "Semantic error: cannot deduce type\n");
    //uncomment on prod - exit(2)\n
    return NULL;
}

/// @brief analyses type coherency of an instruction (or multiple nested instructions)
/// @param instr pointer to the instruction node
/// @param localVars pointer to DeclVars of a current function
/// @param localtypes a null-terminated-array of locally declared structure types (must be non-null)
static void analyseInst(Node* instr, Node* localVars, Node** localtypes){

}

/// @brief provides sematic analysis for a given function
/// @param func pointer to DeclFonct node of the function to analyse
static void analyse_func(Node* func){
    assert(func != NULL);

    Node* local_vars = func->firstChild->nextSibling->firstChild;
    Node* localtypes[MAX_TYPES];
    for(int i = 0; i < MAX_TYPES; i++) glob_types[i] = NULL;
    analyse_variables(local_vars, localtypes);

    Node* curInstr = local_vars -> nextSibling -> firstChild;
    while (curInstr != NULL)
    {
        analyse_func(curInstr);
        curInstr = curInstr ->nextSibling;
    }
}

/// @brief provides a semantic analyse of the program
/// if a semantic error is found, exits with code 2
/// @param tree pointer to the root of programm tree (Prog)
extern void analyse_semantics(Node* tree){
    assert(tree != NULL);

    glob_vars = tree->firstChild;
    /*functs = tree->firstChild->nextSibling;
    for(int i = 0; i < MAX_TYPES; i++) glob_types[i] = NULL;
    analyse_variables(glob_vars, glob_types);
    /* check for "int main" is present in the tree, stop if not

    Node* cur_func = functs->firstChild; //current "DeclFonct" node
    while (cur_func != NULL)
    {
        analyse_func(cur_func);
        cur_func = cur_func ->nextSibling;
    }*/
    

    return;
}