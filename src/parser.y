%{
#include <stdio.h>
#include <stdlib.h>
#include <tree.h>
extern int yylex();
extern int yyparse();
extern FILE* yyin;

void yyerror(char* s);
Node* expr;

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

%%
Prog:  DeclVars DeclFoncts
    ;
DeclVars:
       DeclVars TYPE Declarateurs ';'
    |
    ;
Declarateurs:
       Declarateurs ',' IDENT
    |  IDENT
    ;
DeclFoncts:
       DeclFoncts DeclFonct
    |  DeclFonct
    ;
DeclFonct:
       EnTeteFonct Corps
    ;
EnTeteFonct:
       TYPE IDENT '(' Parametres ')'
    |  VOID IDENT '(' Parametres ')'
    ;
Parametres:
       VOID
    |  ListTypVar
    ;
ListTypVar:
       ListTypVar ',' TYPE IDENT
    |  TYPE IDENT
    ;
Corps: '{' DeclVars SuiteInstr '}'
    ;
SuiteInstr:
       SuiteInstr Instr
    |
    ;
Instr:
       IDENT '=' Exp ';'
    |  IF '(' Exp ')' Instr
    |  IF '(' Exp ')' Instr ELSE Instr
    |  WHILE '(' Exp ')' Instr
    |  IDENT '(' Arguments  ')' ';'
    |  RETURN Exp ';'
    |  RETURN ';'
    |  '{' SuiteInstr '}'
    |  ';'
    ;
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
    |  IDENT
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
void yyerror(char* s){
    fprintf(stderr, "Syntax error on line %d: %s\n", linenum, s);
	//deleteTree(expr);
	exit(1);
}

int main() {
	yyin = stdin;

	do {
		yyparse();
	} while(!feof(yyin));

	//printTree(expr);
	//deleteTree(expr);
	printf("Parsed successfully\n");
	return 0;
}