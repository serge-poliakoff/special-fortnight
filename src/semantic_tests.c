#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <tree.h>
#include <semantics.h>

/// @brief checks that check_exp correctly determines type of int or char literral
/// @return 1 if test passed 0 otherwise
int exprTest1(){
    // Test int literal
    Node n1;
    n1.label.type = INT;
    n1.label.value.number = 42;
    n1.firstChild = n1.nextSibling = NULL;
    assert(strcmp(check_type(&n1, NULL, NULL), "int") == 0);

    // Test char literal
    Node n2;
    n2.label.type = CHAR;
    n2.label.value.character = 'a';
    n2.firstChild = n2.nextSibling = NULL;
    assert(strcmp(check_type(&n2, NULL, NULL), "char") == 0);
    return 1;
}

/// @brief checks that check_exp can return a type of basic arithmetical operators
/// @return 1 if test passed 0 otherwise
int exprTest2(){
    // Test int + int
    Node left, right, op;
    left.label.type = INT;
    left.label.value.number = 1;
    left.firstChild = left.nextSibling = NULL;
    right.label.type = INT;
    right.label.value.number = 2;
    right.firstChild = right.nextSibling = NULL;
    op.label.type = OP;
    op.label.value.id = "+";
    op.firstChild = &left;
    left.nextSibling = &right;
    right.nextSibling = NULL;
    assert(strcmp(check_type(&op, NULL, NULL), "int") == 0);

    // Test char + int (should promote to int)
    left.label.type = CHAR;
    left.label.value.character = 'b';
    right.label.type = INT;
    right.label.value.number = 3;
    assert(strcmp(check_type(&op, NULL, NULL), "int") == 0);
    return 1;
}

/// @brief checks that check_exp can find a type of a local variable
/// @return 1 if test passed 0 otherwise
int exprTest3(){
    // Setup: DeclVars node with int x;
    Node declVars, typeNode, identNode;
    declVars.label.type = KEYWORD;
    declVars.label.value.label = DeclVars;
    declVars.firstChild = &typeNode;
    declVars.nextSibling = NULL;
    typeNode.label.type = TP;
    typeNode.label.value.id = "int";
    typeNode.firstChild = &identNode;
    typeNode.nextSibling = NULL;
    identNode.label.type = ID;
    identNode.label.value.id = "x";
    identNode.firstChild = identNode.nextSibling = NULL;

    // Test lookup of variable x
    Node varNode;
    varNode.label.type = ID;
    varNode.label.value.id = "x";
    varNode.firstChild = varNode.nextSibling = NULL;
    assert(strcmp(check_type(&varNode, &declVars, NULL), "int") == 0);
    return 1;
}

/// @brief checks that check_exp can find a field of a locally defined struct variable
/// @return 1 if test passed 0 otherwise
int exprTest4(){
    // Define struct rect { int w; int h; }
    Node structType, fieldW, fieldH;
    structType.label.type = TP;
    structType.label.value.id = "rect";
    structType.firstChild = &fieldW;
    structType.nextSibling = NULL;
    fieldW.label.type = TP;
    fieldW.label.value.id = "int";
    fieldW.firstChild = &fieldH;
    fieldW.nextSibling = NULL;
    fieldH.label.type = ID;
    fieldH.label.value.id = "w";
    fieldH.firstChild = NULL;
    fieldH.nextSibling = NULL;
    // Only one field for simplicity, add h as a sibling
    Node fieldH2;
    fieldH2.label.type = ID;
    fieldH2.label.value.id = "h";
    fieldH2.firstChild = NULL;
    fieldH2.nextSibling = NULL;
    fieldH.nextSibling = &fieldH2;

    // DeclVars: rect a;
    Node declVars, typeNode, identNode;
    declVars.label.type = KEYWORD;
    declVars.label.value.label = DeclVars;
    declVars.firstChild = &typeNode;
    declVars.nextSibling = NULL;
    typeNode.label.type = TP;
    typeNode.label.value.id = "rect";
    typeNode.firstChild = &identNode;
    typeNode.nextSibling = NULL;
    identNode.label.type = ID;
    identNode.label.value.id = "a";
    identNode.firstChild = identNode.nextSibling = NULL;

    // localtypes array
    Node* localtypes[2] = {&structType, NULL};

    // Build IdExpr: a.w
    Node fieldNode;
    fieldNode.label.type = ID;
    fieldNode.label.value.id = "w";
    fieldNode.firstChild = NULL;
    fieldNode.nextSibling = NULL;
    Node aNode;
    aNode.label.type = ID;
    aNode.label.value.id = "a";
    aNode.firstChild = &fieldNode;
    aNode.nextSibling = NULL;

    // Should resolve to int
    assert(strcmp(check_type(&aNode, &declVars, localtypes), "int") == 0);
    return 1;
}

/// @brief checks that check_type returns NULL for struct/int division
/// @return 1 if test passed 0 otherwise
int exprTestErr1(){
    // struct rect { int w; } r; int x;
    Node structType, fieldW;
    structType.label.type = TP;
    structType.label.value.id = "rect";
    structType.firstChild = &fieldW;
    structType.nextSibling = NULL;
    fieldW.label.type = TP;
    fieldW.label.value.id = "int";
    fieldW.firstChild = NULL;
    fieldW.nextSibling = NULL;

    Node declVars, typeRect, identR, typeInt, identX;
    declVars.label.type = KEYWORD;
    declVars.label.value.label = DeclVars;
    declVars.firstChild = &typeRect;
    declVars.nextSibling = NULL;
    typeRect.label.type = TP;
    typeRect.label.value.id = "rect";
    typeRect.firstChild = &identR;
    typeRect.nextSibling = &typeInt;
    identR.label.type = ID;
    identR.label.value.id = "r";
    identR.firstChild = identR.nextSibling = NULL;
    typeInt.label.type = TP;
    typeInt.label.value.id = "int";
    typeInt.firstChild = &identX;
    typeInt.nextSibling = NULL;
    identX.label.type = ID;
    identX.label.value.id = "x";
    identX.firstChild = identX.nextSibling = NULL;

    Node* localtypes[2] = {&structType, NULL};

    // r / x
    Node rNode, xNode, opNode;
    rNode.label.type = ID; rNode.label.value.id = "r"; rNode.firstChild = rNode.nextSibling = NULL;
    xNode.label.type = ID; xNode.label.value.id = "x"; xNode.firstChild = xNode.nextSibling = NULL;
    opNode.label.type = OP; opNode.label.value.id = "/";
    opNode.firstChild = &rNode; rNode.nextSibling = &xNode; xNode.nextSibling = NULL;
    assert(check_type(&opNode, &declVars, localtypes) == NULL);
    return 1;
}

/// @brief checks that check_type returns NULL for undeclared variable
/// @return 1 if test passed 0 otherwise
int exprTestErr2(){
    Node prog;
    Node declVars;
    declVars.label.type = KEYWORD;
    declVars.label.value.label = DeclVars;
    declVars.firstChild = NULL;
    declVars.nextSibling = NULL;
    Node varNode;
    varNode.label.type = ID;
    varNode.label.value.id = "notfound";
    varNode.firstChild = varNode.nextSibling = NULL;

    prog.firstChild = &declVars;
    
    analyse_semantics(&prog);

    assert(check_type(&varNode, &declVars, NULL) == NULL);
    return 1;
}

/// todo: why no error in console ?
/// @brief checks that check_type returns NULL for non-existing struct field
/// @return 1 if test passed 0 otherwise
int exprTestErr3(){
    // struct rect { int w; };
    Node structType, fieldW, fieldID;
    structType.label.type = TP;
    structType.label.value.id = "rect";
    structType.firstChild = &fieldW;
    structType.nextSibling = NULL;
    fieldW.label.type = TP;
    fieldW.label.value.id = "int";
    fieldW.firstChild = &fieldID;
    fieldW.nextSibling = NULL;
    fieldID.label.type = ID;
    fieldID.label.value.id = "w";
    fieldID.firstChild = fieldID.nextSibling = NULL;

    Node declVars, typeRect, identR;
    declVars.label.type = KEYWORD;
    declVars.label.value.label = DeclVars;
    declVars.firstChild = &typeRect;
    declVars.nextSibling = NULL;
    typeRect.label.type = TP;
    typeRect.label.value.id = "rect";
    typeRect.firstChild = &identR;
    typeRect.nextSibling = NULL;
    identR.label.type = ID;
    identR.label.value.id = "r";
    identR.firstChild = identR.nextSibling = NULL;

    Node* localtypes[2] = {&structType, NULL};

    // r.nonexistent
    Node fieldNode, rNode;
    fieldNode.label.type = ID;
    fieldNode.label.value.id = "nonexistent";
    fieldNode.firstChild = NULL;
    fieldNode.nextSibling = NULL;
    rNode.label.type = ID;
    rNode.label.value.id = "r";
    rNode.firstChild = &fieldNode;
    rNode.nextSibling = NULL;
    assert(check_type(&rNode, &declVars, localtypes) == NULL);
    return 1;
}

/// @brief Validates a call to a int function with no parameters
/// @return 1 if test passed 0 otherwise
int funcsTest1(){
    // Program: int testFunc(void) {}
    Node prog, declVars, declFoncts, declFonct, header, typeNode, identNode, params, body;
    Node voidKeyword; 
    prog.label.type = KEYWORD; prog.label.value.label = Prog;
    prog.firstChild = &declVars; prog.nextSibling = NULL;

    declVars.label.type = KEYWORD; declVars.label.value.label = DeclVars;
    declVars.firstChild = NULL; declVars.nextSibling = &declFoncts;

    declFoncts.label.type = KEYWORD; declFoncts.label.value.label = DeclFoncts;
    declFoncts.firstChild = &declFonct; declFoncts.nextSibling = NULL;

    declFonct.label.type = KEYWORD; declFonct.label.value.label = DeclFonct;
    declFonct.firstChild = &header; declFonct.nextSibling = NULL;

    header.label.type = KEYWORD; header.label.value.label = EnTeteFonct;
    header.firstChild = &typeNode; header.nextSibling = &body;

    typeNode.label.type = TP; typeNode.label.value.id = "int";
    typeNode.firstChild = NULL; typeNode.nextSibling = &identNode;

    identNode.label.type = ID; identNode.label.value.id = "testFunc";
    identNode.firstChild = NULL; identNode.nextSibling = &params;

    params.label.type = KEYWORD; params.label.value.label = Parametres;
    params.firstChild = &voidKeyword; params.nextSibling = NULL;

    voidKeyword.label.type = KEYWORD; voidKeyword.label.value.label = Void;
    voidKeyword.firstChild = voidKeyword.nextSibling = NULL;

    body.label.type = KEYWORD; body.label.value.label = Corps;
    body.firstChild = NULL; body.nextSibling = NULL;

    

    // Set up global variables
    analyse_semantics(&prog);
    // Call node: testFunc()
    Node callNode, argsNode;
    callNode.label.type = ID; callNode.label.value.id = "testFunc";
    callNode.firstChild = &argsNode; callNode.nextSibling = NULL;
    argsNode.label.type = KEYWORD; argsNode.label.value.label = Arguments;
    argsNode.firstChild = NULL; argsNode.nextSibling = NULL;

    
    // Should return "int"
    assert(strcmp(check_function_call(&callNode, NULL, NULL), "int") == 0);
    return 1;
}

/// @brief Validates a call to a void function with int and char parameters
/// @return 1 if test passed 0 otherwise
int funcsTest2(){
    // Program: void testFunc(int a, char b) {}
    Node prog, declVars, declFoncts, declFonct, header, typeNode, identNode, params, param1, param1id, param2, param2id, body;
    prog.label.type = KEYWORD; prog.label.value.label = Prog;
    prog.firstChild = &declVars; prog.nextSibling = NULL;
    declVars.label.type = KEYWORD; declVars.label.value.label = DeclVars;
    declVars.firstChild = NULL; declVars.nextSibling = &declFoncts;
    declFoncts.label.type = KEYWORD; declFoncts.label.value.label = DeclFoncts;
    declFoncts.firstChild = &declFonct; declFoncts.nextSibling = NULL;
    declFonct.label.type = KEYWORD; declFonct.label.value.label = DeclFonct;
    declFonct.firstChild = &header; declFonct.nextSibling = NULL;
    header.label.type = KEYWORD; header.label.value.label = EnTeteFonct;
    header.firstChild = &typeNode; header.nextSibling = &body;

    typeNode.label.type = KEYWORD; typeNode.label.value.label = Void;
    typeNode.firstChild = NULL; typeNode.nextSibling = &identNode;

    identNode.label.type = ID; identNode.label.value.id = "testFunc";
    identNode.firstChild = NULL; identNode.nextSibling = &params;

    params.label.type = KEYWORD; params.label.value.label = Parametres;
    params.firstChild = &param1; params.nextSibling = NULL;
    param1.label.type = TP; param1.label.value.id = "int";
    param1.firstChild = &param1id; param1.nextSibling = &param2;
    param1id.label.type = ID; param1id.label.value.id = "a";
    param1id.firstChild = param1id.nextSibling = NULL;
    param2.label.type = TP; param2.label.value.id = "char";
    param2.firstChild = &param2id; param2.nextSibling = NULL;
    param2id.label.type = ID; param2id.label.value.id = "b";
    param2id.firstChild = param2id.nextSibling = NULL;
    body.label.type = KEYWORD; body.label.value.label = Corps;
    body.firstChild = NULL; body.nextSibling = NULL;
    // Set up global variables
    analyse_semantics(&prog);
    // Call node: testFunc(1, 'c')
    Node callNode, argsNode, arg1, arg2;
    callNode.label.type = ID; callNode.label.value.id = "testFunc";
    callNode.firstChild = &argsNode; callNode.nextSibling = NULL;
    argsNode.label.type = KEYWORD; argsNode.label.value.label = Arguments;
    argsNode.firstChild = &arg1; argsNode.nextSibling = NULL;
    arg1.label.type = INT; arg1.label.value.number = 1; arg1.firstChild = NULL; arg1.nextSibling = &arg2;
    arg2.label.type = CHAR; arg2.label.value.character = 'c'; arg2.firstChild = arg2.nextSibling = NULL;
    // Should return NULL (void)
    assert(check_function_call(&callNode, NULL, NULL) == NULL);
    // Call node: testFunc('a', 'b')
    Node callNode2, argsNode2, arg3, arg4;
    callNode2.label.type = ID; callNode2.label.value.id = "testFunc";
    callNode2.firstChild = &argsNode2; callNode2.nextSibling = NULL;
    argsNode2.label.type = KEYWORD; argsNode2.label.value.label = Arguments;
    argsNode2.firstChild = &arg3; argsNode2.nextSibling = NULL;
    arg3.label.type = CHAR; arg3.label.value.character = 'a'; arg3.firstChild = NULL; arg3.nextSibling = &arg4;
    arg4.label.type = CHAR; arg4.label.value.character = 'b'; arg4.firstChild = arg4.nextSibling = NULL;
    assert(check_function_call(&callNode2, NULL, NULL) == NULL);
    //printf("Test 2 passed\n");
    return 1;
}

/// @brief Validates a call to a char function with structure typed parameter
/// @return 1 if test passed 0 otherwise
int funcsTest3(){
    // struct s1 { char c; }
    Node structType, fieldC;
    structType.label.type = TP;
    structType.label.value.id = "s1";
    structType.firstChild = &fieldC;
    structType.nextSibling = NULL;
    fieldC.label.type = TP;
    fieldC.label.value.id = "char";
    fieldC.firstChild = NULL;
    fieldC.nextSibling = NULL;

    // char testFunc(struct s1 param)
    Node prog, declVars, declFoncts, declFonct, header, typeNode, identNode, params, param1, param1id, body;
    prog.label.type = KEYWORD; prog.label.value.label = Prog;
    prog.firstChild = &declVars; prog.nextSibling = NULL;
    declVars.label.type = KEYWORD; declVars.label.value.label = DeclVars;
    declVars.firstChild = NULL; declVars.nextSibling = &declFoncts;
    declFoncts.label.type = KEYWORD; declFoncts.label.value.label = DeclFoncts;
    declFoncts.firstChild = &declFonct; declFoncts.nextSibling = NULL;
    declFonct.label.type = KEYWORD; declFonct.label.value.label = DeclFonct;
    declFonct.firstChild = &header; declFonct.nextSibling = NULL;
    header.label.type = KEYWORD; header.label.value.label = EnTeteFonct;
    header.firstChild = &typeNode; header.nextSibling = &body;

    typeNode.label.type = TP; typeNode.label.value.id = "char";
    typeNode.firstChild = NULL; typeNode.nextSibling = &identNode;

    identNode.label.type = ID; identNode.label.value.id = "testFunc";
    identNode.firstChild = NULL; identNode.nextSibling = &params;

    params.label.type = KEYWORD; params.label.value.label = Parametres;
    params.firstChild = &param1; params.nextSibling = NULL;
    param1.label.type = TP; param1.label.value.id = "s1";
    param1.firstChild = &param1id; param1.nextSibling = NULL;
    param1id.label.type = ID; param1id.label.value.id = "param";
    param1id.firstChild = param1id.nextSibling = NULL;
    body.label.type = KEYWORD; body.label.value.label = Corps;
    body.firstChild = NULL; body.nextSibling = NULL;

    // Local DeclVar: s1 ex;
    Node localDeclVars, localTypeNode, localIdentNode;
    localDeclVars.label.type = KEYWORD; localDeclVars.label.value.label = DeclVars;
    localDeclVars.firstChild = &localTypeNode; localDeclVars.nextSibling = NULL;
    localTypeNode.label.type = TP; localTypeNode.label.value.id = "s1";
    localTypeNode.firstChild = &localIdentNode; localTypeNode.nextSibling = NULL;
    localIdentNode.label.type = ID; localIdentNode.label.value.id = "ex";
    localIdentNode.firstChild = localIdentNode.nextSibling = NULL;
    Node* localtypes[2] = {&structType, NULL};
    // Call node: testFunc(ex)
    Node callNode, argsNode, arg1;
    callNode.label.type = ID; callNode.label.value.id = "testFunc";
    callNode.firstChild = &argsNode; callNode.nextSibling = NULL;
    argsNode.label.type = KEYWORD; argsNode.label.value.label = Arguments;
    argsNode.firstChild = &arg1; argsNode.nextSibling = NULL;
    arg1.label.type = ID; arg1.label.value.id = "ex"; arg1.firstChild = arg1.nextSibling = NULL;

    /*printf("Prog tree: \n");
    printTree(&prog);

    printf("\nCall: \n");
    printTree(&callNode);*/

    // Set up global variables
    analyse_semantics(&prog);
    
    // Should return "char"
    assert(strcmp(check_function_call(&callNode, &localDeclVars, localtypes), "char") == 0);
    return 1;
}


int main(){
    int count_good = 0;
    count_good += exprTest1();
    count_good += exprTest2();
    count_good += exprTest3();
    count_good += exprTest4();
    count_good += exprTestErr1();
    count_good += exprTestErr2();
    count_good += exprTestErr3();
    count_good += funcsTest1();
    count_good += funcsTest2();
    count_good += funcsTest3();
    printf("Semantic expr tests passed: %d/10\n", count_good);
    return 0;
}