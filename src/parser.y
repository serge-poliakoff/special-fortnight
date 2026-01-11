%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tree.h>
extern int yylex();
extern int yyparse();
extern FILE* yyin;

void yyerror(const char* s);
Node* prog;

%}

%error-verbose
%verbose

%define api.value.type {Node*}
%token CHARACTER	/* char value */
%token NUM			/* int value */
%token IDENT 		/* function/variable name */
%token TYPE 		/*type simple: int or char */
%token EQ 			/* egality (’==’) and inegality (’!=’) operators */
%token ORDER		/* comparaison operators ’<’, ’<=’, ’>’ and ’>=’ */
%token ADDSUB 		/* ’+’ et ’-’ (binairy or unary) */
%token DIVSTAR 		/* ’*’, ’/’ and ’%’ */
%token OR
%token AND

%token VOID
%token IF
%token WHILE
%token RETURN
%token ELSE

%token STRUCT

%%
Prog:  DeclVars DeclFoncts {
    prog = makeNode(Prog);
    addChild(prog, $1);
    addChild(prog, $2);
    }
    ;
DeclVars:
       DeclVars TYPE Declarateurs ';' {Node *type = makeNode(Type);
            addChild($1, type); addChild(type, $3); $$ = $1;}
    |  DeclVars STRUCT IDENT '{' DeclStructVars '}' ';'
    |  DeclVars STRUCT IDENT Declarateurs ';'
    |  {Node* cur = makeNode(DeclVars); $$ = cur;}
    ;
DeclStructVars:
       DeclStructVars TYPE Declarateurs ';'
    |  DeclStructVars STRUCT IDENT Declarateurs ';'
    |
    ;
Declarateurs:
       Declarateurs ',' IDENT {Node* id = makeNode(Ident); addSibling($1, id);}
    |  IDENT {Node* cur = makeNode(Ident); $$ = cur;}
    ;
DeclFoncts:
       DeclFoncts DeclFonct {addChild($1, $2); $$ = $1;}
    |  DeclFonct {Node* cur = makeNode(DeclFoncts); addChild(cur, $1); $$ = cur;}
    ;
DeclFonct:
       EnTeteFonct Corps {
        Node *cur = makeNode(DeclFonct);
        addChild(cur, $1); addChild(cur, $2);
        $$ = cur;
       }
    ;
EnTeteFonct:
       TYPE IDENT '(' Parametres ')' {
        Node* cur = makeNode(EnTeteFonct);
        Node* type = makeNode(Type);
        Node* ident = makeNode(Ident);
        addChild(cur, type); addChild(cur, ident); addChild(cur, $4);
        $$ = cur;
       }
    |  VOID IDENT '(' Parametres ')'{
        Node* cur = makeNode(EnTeteFonct);
        Node* type = makeNode(Void);
        Node* ident = makeNode(Ident);
        addChild(cur, type); addChild(cur, ident); addChild(cur, $4);
        $$ = cur;
       }
    |  STRUCT IDENT IDENT '(' Parametres ')'{
        Node* cur = makeNode(EnTeteFonct);
        Node* type = makeNode(Struct);
        Node* typeName = makeNode(Ident);
        Node* ident = makeNode(Ident);
        addChild(cur, type); addChild(type, typeName); addChild(cur, ident); addChild(cur, $5);
        $$ = cur;
       }
    ;
Parametres:
       VOID {
        Node* cur = makeNode(Parametres);
        Node* type = makeNode(Void);
        addChild(cur, type);
        $$ = cur;
       }
    |  ListTypVar {
        Node* cur = makeNode(Parametres);
        addChild(cur, $1);
        $$ = cur;
       }
    ;
ListTypVar:
       ListTypVar ',' TYPE IDENT {
        Node* type = makeNode(Type);
        Node* ident = makeNode(Ident);
        addChild($1, type); addChild(type, ident);
        $$ = $1;
        }
    |  ListTypVar ',' STRUCT IDENT IDENT {
        Node* type = makeNode(Struct);
        Node* typeName = makeNode(Ident);
        Node* ident = makeNode(Ident);
        addChild($1, type); addChild(type, typeName); addChild(typeName, ident);
        $$ = $1;
       }
    |  STRUCT IDENT IDENT {
        Node* cur = makeNode(ListTypVar);
        Node* type = makeNode(Struct);
        Node* typeName = makeNode(Ident);
        Node* ident = makeNode(Ident);
        addChild(cur, type); addChild(type, typeName); addChild(typeName, ident);
        $$ = cur;
       }
    |  TYPE IDENT {
        Node* cur = makeNode(ListTypVar);
        Node* type = makeNode(Type);
        Node* ident = makeNode(Ident);
        addChild(cur, type); addChild(type, ident);
        $$ = cur;
        }
    ;
Corps: '{' DeclVars SuiteInstr '}'
    ;
SuiteInstr:
       SuiteInstr Instr
    |
    ;
Instr:
       IdExpr '=' Exp ';'
    |  IF '(' Exp ')' Instr
    |  IF '(' Exp ')' Instr ELSE Instr
    |  WHILE '(' Exp ')' Instr
    |  IDENT '(' Arguments  ')' ';'
    |  RETURN Exp ';'
    |  RETURN ';'
    |  '{' SuiteInstr '}'
    |  ';'
    ;
IdExpr:
       IDENT
    |  IDENT '.' IDENT
Exp :  Exp OR TB
    |  TB
    ;
TB  :  TB AND FB
    |  FB
    ;
FB  :  FB EQ M
    |  M
    ;
M   :  M ORDER E
    |  E
    ;
E   :  E ADDSUB T
    |  T
    ;    
T   :  T DIVSTAR F 
    |  F
    ;
F   :  ADDSUB F
    |  '!' F
    |  '(' Exp ')'
    |  NUM
    |  CHARACTER
    |  IdExpr
    |  IDENT '(' Arguments  ')'
    ;
Arguments:
       ListExp
    |
    ;
ListExp:
       ListExp ',' Exp
    |  Exp
    ;
%%

extern int linenum;
void yyerror(const char* s){
    fprintf(stderr, "Syntax error on line %d: %s\n", linenum, s);
	deleteTree(prog);
	exit(1);
}

int main(int argc, char **argv) {
	yyin = stdin;

	do {
		yyparse();
	} while(!feof(yyin));

    if(argc < 1 || strcmp(argv[1], "--dry-run") != 0)
	    printTree(prog);
	deleteTree(prog);
	printf("Parsed successfully\n");
	return 0;
}