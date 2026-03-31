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

/// @brief checks that check_type returns NULL for non-existing struct field
int exprTestErr3(){
    // struct rect { int w; } r;
    Node structType, fieldW;
    structType.label.type = TP;
    structType.label.value.id = "rect";
    structType.firstChild = &fieldW;
    structType.nextSibling = NULL;
    fieldW.label.type = TP;
    fieldW.label.value.id = "int";
    fieldW.firstChild = NULL;
    fieldW.nextSibling = NULL;

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

int main(){
    int count_good = 0;
    count_good += exprTest1();
    count_good += exprTest2();
    count_good += exprTest3();
    count_good += exprTest4();
    count_good += exprTestErr1();
    count_good += exprTestErr2();
    count_good += exprTestErr3();
    printf("Semantic expr tests passed: %d/7\n", count_good);
    return 0;
}