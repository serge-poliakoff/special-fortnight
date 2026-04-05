#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <tree.h>
#define MAX_TYPES 100

Node* glob_vars;    //pointer to program's global VarDecl
Node* glob_types[MAX_TYPES];    //global custom types (structs)
Node* functs;   //pointer to DeclFoncts node of tree

extern char* check_type(Node* Exp, Node* localVars, Node** localtypes);

/// @brief helper function for expression type analysing
/// @param typeName name of a type
/// @return 1 if type is int or char, 0 otherwise 
static int int_or_char(char* typeName){
    return ! (strcmp(typeName, "int") && strcmp(typeName, "char"));
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

/// @brief analyses VarDecl node of programm or function and collects all structure type
/// declarations into a given typetable
/// @param declVars pointer to VarDecl node
/// @param typetable pointer to a preinitiallized type table of MAX_TYPES size (supposed initialized to all nulls)
extern void analyse_variables(Node* declVars, Node** typetable){
    if (!declVars) return;
    Node* cur = declVars->firstChild;
    int type_idx = 0;

    for (; cur; cur = cur->nextSibling) {
        if (cur->label.type == TP) {
            // Check if type is standart or exists in typetable or glob_types
            char* type_name = cur->label.value.id;
            printf("Variable %s with type %s found\n", type_name, cur->firstChild->label.value.id);
            if (int_or_char(type_name)) continue;

            Node* found = lookup_type_between(type_name, glob_types);
            found = found ? found : lookup_type_between(type_name, typetable);

            if (found == NULL) {
                fprintf(stderr, "Semantic error: %s is declared with an undeclared type %s\n",
                    cur->firstChild->label.value.id, type_name);
                return;
                //exit(2);
            }
        } else if (cur->label.type == KEYWORD && cur->label.value.label == Struct) {
            // Add struct type to typetable
            Node* struct_type = cur->firstChild; // Should be TP node for struct name
            // Add to typetable
            typetable[type_idx++] = struct_type;
            printf("Structure %s declaration found\n", struct_type->label.value.id);
            analyse_variables(struct_type, typetable);
        }
    }
}

// Helper: check function call arguments (stub)
extern char* check_function_call(Node* funcNode, Node* localVars, Node** localtypes) {
    // funcNode: ID node with function name, arguments: Arguments node
    char* func_name = funcNode->label.value.id;
    Node* arguments = funcNode->firstChild;

    // searching function in program
    Node* cur = functs ? functs->firstChild : NULL;
    Node* found_func = NULL;
    while (cur) {
        // Each DeclFonct: firstChild is EnTeteFonct
        Node* header = cur->firstChild;
        if (header && header->firstChild && header->firstChild->nextSibling) {
            Node* nameNode = header->firstChild->nextSibling;
            if (nameNode->label.type == ID && strcmp(nameNode->label.value.id, func_name) == 0) {
                found_func = cur;
                break;
            }
        }
        cur = cur->nextSibling;
    }
    if (!found_func) {
        fprintf(stderr, "Semantic error: function '%s' not found\n", func_name);
        exit(2);
    }

    // Get parameter list from EnTeteFonct
    Node* header = found_func->firstChild;
    char* returnType = header->firstChild->label.type == TP ?
        header->firstChild->label.value.id : NULL; // return this if arguments are valid (NULL for void functions)
    
    Node* params = header->firstChild->nextSibling->nextSibling; // Parametres node
    Node* param = params->firstChild;
    // validate function without arguments
    if (param -> label.type == KEYWORD && param -> label.value.label == Void){
        if (arguments->firstChild == NULL){
            return returnType;  // function's type
        }else{
            fprintf(stderr, "Semantic error: function '%s' has no parameters\n", func_name);
            exit(2);
        }
    }

    // Get argument list from Arguments node
    Node* arg = arguments->firstChild;
    int param_count = 0, arg_count = 0;
    // Count params and args, and check types
    while (param && arg) {
        // Each param: type of parameter, its ID in the first child
        Node* param_type_node = param;
        Node* param_id_node = param_type_node->firstChild;
        char* param_type = param_type_node->label.value.id;
        char* arg_type = check_type(arg, localVars, localtypes);
        if (strcmp(param_type, "int") == 0) {
            if (!int_or_char(arg_type)) {
                fprintf(stderr, "Semantic error: argument %d of '%s' must be int or char, got %s\n", param_count+1, func_name, arg_type);
                exit(2);
            }
        } else if (strcmp(param_type, "char") == 0) {
            if (strcmp(arg_type, "char") == 0) {
                // ok
            } else if (strcmp(arg_type, "int") == 0) {
                fprintf(stderr, "Warning: passing int to char parameter %d of '%s'\n", param_count+1, func_name);
            } else {
                fprintf(stderr, "Semantic error: argument %d of '%s' must be char, got %s\n", param_count+1, func_name, arg_type);
                exit(2);
            }
        } else {
            // struct or other type
            if (strcmp(param_type, arg_type) != 0) {
                fprintf(stderr, "Semantic error: argument %d of '%s' must be %s, got %s\n", param_count+1, func_name, param_type, arg_type);
                exit(2);
            }
        }
        param = param->nextSibling;
        arg = arg->nextSibling;
        param_count++;
        arg_count++;
    }
    // Check for extra/missing arguments
    // todo: rewrite later: calculate number of arguments and parameters
    if (param || arg) {
        fprintf(stderr, "Semantic error: function '%s' expects %d arguments, got %d\n", func_name, param_count + (param != NULL), arg_count + (arg != NULL));
        exit(2);
    }
    
    return returnType;
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
                return check_function_call(Exp, localVars, localtypes);
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
/// @param returnType return type of current function; NULL if void
static void analyseInst(Node* instr, Node* localVars, Node** localtypes, char* returnType){
    if (!instr) return;
    // Handle keyword instructions first
    if (instr->firstChild && instr->firstChild->label.type == KEYWORD) {
        Node* kw = instr->firstChild;
        switch (kw->label.value.label) {
            case Return: {
                Node* retExpr = kw->firstChild;
                if (returnType == NULL) {
                    // void function: should not return a value
                    if (retExpr) {
                        fprintf(stderr, "Semantic error: void function should not return a value\n");
                        exit(2);
                    }
                } else {
                    if (!retExpr) {
                        fprintf(stderr, "Semantic error: non-void function must return a value\n");
                        exit(2);
                    }
                    char* exprType = check_type(retExpr, localVars, localtypes);
                    // todo: remove in prod: check_type must throw itself if undeclared type
                    if (!exprType) {
                        fprintf(stderr, "Semantic error: cannot deduce return expression type\n");
                        exit(2);
                    }
                    if (strcmp(returnType, "int") == 0) {
                        if (!int_or_char(exprType)) {
                            fprintf(stderr, "Semantic error: return type must be int or char, got %s\n", exprType);
                            exit(2);
                        }
                    } else if (strcmp(returnType, "char") == 0) {
                        if (strcmp(exprType, "char") == 0) {
                            // ok
                        } else if (strcmp(exprType, "int") == 0) {
                            fprintf(stderr, "Warning: returning int from char function\n");
                        } else {
                            fprintf(stderr, "Semantic error: return type must be char, got %s\n", exprType);
                            exit(2);
                        }
                    } else {
                        if (strcmp(exprType, returnType) != 0) {
                            fprintf(stderr, "Semantic error: return type must be %s, got %s\n", returnType, exprType);
                            exit(2);
                        }
                    }
                }
                break;
            }
            case If: {
                Node* cond = kw->firstChild;
                Node* thenInstr = cond ? cond->nextSibling : NULL;

                char* condType = check_type(cond, localVars, localtypes);
                if (!int_or_char(condType)) {
                    fprintf(stderr, "Semantic error: if condition must be int or char, got %s\n", condType);
                    exit(2);
                }
                if (thenInstr)
                    analyseInst(thenInstr, localVars, localtypes, returnType);
                // Check for else block
                Node* elseNode = kw->nextSibling;
                if (elseNode && elseNode->label.type == KEYWORD && elseNode->label.value.label == Else) {
                    Node* elseInstr = elseNode->firstChild;
                    if (elseInstr) analyseInst(elseInstr, localVars, localtypes, returnType);
                }
                break;
            }
            case While: {
                Node* cond = kw->nextSibling;
                Node* bodyInstr = cond ? cond->nextSibling : NULL;
                if (!cond) {
                    fprintf(stderr, "Semantic error: malformed while instruction\n");
                    exit(2);
                }
                char* condType = check_type(cond, localVars, localtypes);
                if (!int_or_char(condType)) {
                    fprintf(stderr, "Semantic error: while condition must be int or char, got %s\n", condType);
                    exit(2);
                }
                if (bodyInstr) analyseInst(bodyInstr, localVars, localtypes, returnType);
                break;
            }
            case SuiteInstr:
                Node* cur = kw -> firstChild;
                while (cur)
                {
                    analyseInst(cur, localVars, localtypes, returnType);
                    cur = cur -> nextSibling;
                }
                break;
            default:
                // todo: implement a funciton in tree.c to print a keyword
                fprintf(stderr, "Sematic analyser error: unknown keyword %d", kw->label.value.label);
                exit(2);
        }
        return;
    }
    // Function call: single child
    if (instr->firstChild && instr->firstChild->nextSibling == NULL) {
        Node* callNode = instr->firstChild;
        // Check if this is a function call (has Arguments child)
        check_function_call(callNode, localVars, localtypes);
        return;
    }
    // Variable assignment: two children
    if (instr->firstChild && instr->firstChild->nextSibling) {
        Node* lhs = instr->firstChild;
        Node* rhs = instr->firstChild->nextSibling;
        char* lhsType = check_type(lhs, localVars, localtypes);
        char* rhsType = check_type(rhs, localVars, localtypes);
        // todo: remove in prod. - checkk_type must throw itself if type cannot be deduced
        if (!lhsType || !rhsType) {
            fprintf(stderr, "Semantic error: cannot deduce type in assignment\n");
            exit(2);
        }
        if (strcmp(lhsType, "int") == 0) {
            if (!int_or_char(rhsType)) {
                fprintf(stderr, "Semantic error: cannot assign %s to int variable\n", rhsType);
                exit(2);
            }
        } else if (strcmp(lhsType, "char") == 0) {
            if (strcmp(rhsType, "char") == 0) {
                // ok
            } else if (strcmp(rhsType, "int") == 0) {
                fprintf(stderr, "Warning: assigning int to char variable\n");
            } else {
                fprintf(stderr, "Semantic error: cannot assign %s to char variable\n", rhsType);
                exit(2);
            }
        } else {
            // struct or other type
            if (strcmp(lhsType, rhsType) != 0) {
                fprintf(stderr, "Semantic error: cannot assign %s to %s variable\n", rhsType, lhsType);
                exit(2);
            }
        }
        return;
    }
    fprintf(stderr, "Semantic analyser error: cannot analyse instruction");
    exit(2);
}

/// @brief provides sematic analysis for a given function
/// @param func pointer to DeclFonct node of the function to analyse
extern void analyse_func(Node* func){
    assert(func != NULL);
    //printTree(func);
    // Analyse function signature: return type and parameter types
    Node* header = func->firstChild; // EnTeteFonct
    Node* returnTypeNode = header->firstChild;
    Node* identNode = returnTypeNode->nextSibling;
    
    printf("analysing function %s\n", identNode->label.value.id);

    // Check return type - the only KEYWORD value is Void and do not need to be checked
    if (returnTypeNode->label.type == TP) {
        char* retType = returnTypeNode->label.value.id;
        if (!int_or_char(retType)) {
            Node* found = lookup_type_between(retType, glob_types);
            if (!found) {
                fprintf(stderr, "Semantic error: in function %s return type '%s' is undeclared\n",
                    identNode->label.value.id, retType);
                //exit(2);
                return;
            }
        }
    }
    // Check parameter types
    Node* paramsNode = identNode->nextSibling;
    Node* param = paramsNode->firstChild;
    while (param) {
        // again Void param can be not checked
        if (param->label.type == TP) {
            char* paramType = param->label.value.id;
            if (!int_or_char(paramType)) {
                Node* found = lookup_type_between(paramType, glob_types);
                if (!found) {
                    fprintf(stderr, "Semantic error: in function %s parameter type '%s' is undeclared\n",
                        identNode->label.value.id, paramType);
                    //exit(2);
                    return;
                }
            }
        }
        param = param->nextSibling;
    }

    Node* local_vars = func->firstChild->nextSibling->firstChild;
    Node* localtypes[MAX_TYPES];
    for(int i = 0; i < MAX_TYPES; i++) localtypes[i] = NULL;
    analyse_variables(local_vars, localtypes);

    //printf("Copying parameters to local vars: ");
    //printTree(paramsNode);   

    param = paramsNode->firstChild;
    while (param)
    {
        if (param->label.type == TP){
            addChild(local_vars, copyTree(param));
        }
        param = param -> nextSibling;
    }
    
    printf("analysing function %s: finished analysing variables\n", identNode->label.value.id);

    Node* curInstr = local_vars -> nextSibling -> firstChild;
    while (curInstr != NULL)
    {
        analyseInst(curInstr, local_vars, localtypes, returnTypeNode->label.value.id);
        curInstr = curInstr ->nextSibling;
    }
    printf("function %s valid\n", identNode->label.value.id);
}

/// @brief provides a semantic analyse of the program
/// if a semantic error is found, exits with code 2
/// @param tree pointer to the root of programm tree (Prog)
extern void analyse_semantics(Node* tree){
    assert(tree != NULL);
    
    glob_vars = tree->firstChild;
    functs = tree->firstChild->nextSibling;
    for(int i = 0; i < MAX_TYPES; i++) glob_types[i] = NULL;
    analyse_variables(glob_vars, glob_types);

    //for(int i = 0; glob_types[i] != NULL; i++) printTree(glob_types[i]);
    /*printf("Semantics : printing functs tree \n");
    printTree(functs);
    printf("\n");*/
    /* check for "int main" is present in the tree, stop if not*/

    Node* cur_func = functs->firstChild; //current "DeclFonct" node
    while (cur_func != NULL)
    {
        analyse_func(cur_func);
        cur_func = cur_func -> nextSibling;
    }

    printf("semantic analysis finished\n");

    return;
}