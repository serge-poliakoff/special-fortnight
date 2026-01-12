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
    |  DeclVars STRUCT IDENT '{' DeclStructVars '}' ';' {
        Node *type = makeNode(Struct); Node *typeName = makeNode(Ident);
        addChild(type, typeName);
        addChild(typeName, $5);
        addChild($1, type);
        $$ = $1;
    }
    |  DeclVars STRUCT IDENT Declarateurs ';' {
        Node *type = makeNode(Struct); Node *typeName = makeNode(Ident);
        addChild($1, type); addChild(type, typeName); addChild(typeName, $4);
        $$ = $1;}
    |  {Node* cur = makeNode(DeclVars); $$ = cur;}
    ;
DeclStructVars:
       DeclStructVars TYPE Declarateurs ';' {Node *type = makeNode(Type);
        addChild($1, type); addChild(type, $3); $$ = $1;}
    |  DeclStructVars STRUCT IDENT Declarateurs ';' {
        Node *type = makeNode(Struct); Node *typeName = makeNode(Ident);
        addChild($1, type); addChild(type, typeName); addChild(typeName, $4);
        $$ = $1;}
    |  {Node* cur = makeNode(DeclStructVars); $$ = cur;}
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
Corps: '{' DeclVars SuiteInstr '}' {
        Node* cur = makeNode(Corps);
        addChild(cur, $2); addChild(cur, $3);
        $$ = cur;
    }
    ;
SuiteInstr:
       SuiteInstr Instr {addChild($1, $2);}
    |   {$$ = makeNode(SuiteInstr);}
    ;
Instr:
       IdExpr '=' Exp ';' {
        Node* cur = makeNode(Instr);
        addChild(cur, $1); addChild(cur, $3);
        $$ = cur;
       }
    |  IF '(' Exp ')' Instr {
        Node* cur = makeNode(Instr);
        Node* ifT = makeNode(If);
        addChild(cur, ifT); addChild(cur, $3); addChild(cur, $5);
        $$ = cur;
    }
    |  IF '(' Exp ')' Instr ELSE Instr {
        Node* cur = makeNode(Instr);
        Node* ifT = makeNode(If);
        Node* elseT = makeNode(Else);
        addChild(cur, ifT); addChild(cur, $3); addChild(cur, $5);
        addChild(cur, elseT); addChild(cur, $7);
        $$ = cur;
    }
    |  WHILE '(' Exp ')' Instr {
        Node* cur = makeNode(Instr);
        Node* whileT = makeNode(While);
        addChild(cur, whileT); addChild(cur, $3); addChild(cur, $5);
        $$ = cur;
    }
    |  IDENT '(' Arguments  ')' ';' {
        Node* cur = makeNode(Instr);
        Node* func = makeNode(Ident);
        addChild(cur, func); addChild(cur, $3);
        $$ = cur;
    }
    |  RETURN Exp ';' {
        Node* cur = makeNode(Instr);
        Node* ret = makeNode(Return);
        addChild(cur, ret); addChild(cur, $2);
        $$ = cur;
       }
    |  RETURN ';' {
        Node* cur = makeNode(Instr);
        Node* ret = makeNode(Return);
        addChild(cur, ret);
        $$ = cur;
       }
    |  '{' SuiteInstr '}' {
        Node* cur = makeNode(Instr);
        addChild(cur, $2);
        $$ = cur;
       }
    |  ';'
    ;
IdExpr:
       IDENT {
        Node* cur = makeNode(IdExpr);
        addChild(cur, makeNode(Ident));
        $$ = cur;
       }
    |  IDENT '.' IDENT {
        Node* cur = makeNode(IdExpr);
        addChild(cur, makeNode(Ident)); addChild(cur, makeNode(Ident));
        $$ = cur;
       }
Exp :  Exp OR TB {Node* oper = makeNode(Or);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  TB   {
        Node *cur = makeNode(Exp);
        addChild(cur, $1);
        $$ = cur;
    }
    ;
TB  :  TB AND FB {Node* oper = makeNode(And);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  FB {Node* cur = makeNode(Tb); addChild(cur, $1); $$ = cur;}
    ;
FB  :  FB EQ M {Node* oper = makeNode(Eq);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  M {Node* cur = makeNode(Fb); addChild(cur, $1); $$ = cur;}
    ;
M   :  M ORDER E {Node* oper = makeNode(Order);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  E {Node* cur = makeNode(M); addChild(cur, $1); $$ = cur;}
    ;
E   :  E ADDSUB T {Node* oper = makeNode(Addsub);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  T {Node* cur = makeNode(E); addChild(cur, $1); $$ = cur;}
    ;    
T   :  T DIVSTAR F {Node* oper = makeNode(Divstar);
        addChild($1, oper); addChild($1, $3);
        $$ = $1;}
    |  F {Node* cur = makeNode(T); addChild(cur, $1); $$ = cur;}
    ;
F   :  ADDSUB F {
        Node* cur = makeNode(F);
        addChild(cur, makeNode(Addsub)); addChild(cur, $2);
        $$ = cur;}
    |  '!' F {Node* cur = makeNode(F); addChild(cur, $2); $$ = cur;}
    |  '(' Exp ')' {Node* cur = makeNode(F); addChild(cur, $2); $$ = cur;}
    |  NUM {Node* cur = makeNode(F); addChild(cur, makeNode(Num)); $$ = cur;}
    |  CHARACTER {Node* cur = makeNode(F); addChild(cur, makeNode(Character)); $$ = cur;}
    |  IdExpr {Node* cur = makeNode(F); addChild(cur, $1); $$ = cur;}
    |  IDENT '(' Arguments  ')' {Node* cur = makeNode(F);
        addChild(cur, makeNode(Ident)); addChild(cur, $3); $$ = cur;}
    ;
Arguments:
       ListExp {
        Node *cur = makeNode(Arguments);
        addChild(cur, $1);
        $$ = cur;
    }
    | {$$ = makeNode(Arguments);}
    ;
ListExp:
       ListExp ',' Exp {
        addChild($1, $3);
        $$ = $1;
    }
    |  Exp {
        Node *cur = makeNode(ListExp);
        addChild(cur, $1);
        $$ = cur;
    }
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

    if(argc > 1){
        if (strcmp(argv[1], "--dry-run") != 0){
            printTree(prog);
        }
    }else{
	    printTree(prog);
    }
	deleteTree(prog);
	printf("Parsed successfully\n");
	return 0;
}