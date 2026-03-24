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

%define parse.error verbose
%verbose


%union {
    tree_label value;
    Node* node;
}

%token <value> CHARACTER /* char value */
%token <value> NUM       /* int value */
%token <value> IDENT     /* function/variable name */
%token <value> TYPE      /* type simple: int or char */
%token <node> EQ                /* egality (’==’) and inegality (’!=’) operators */
%token <node> ORDER             /* comparaison operators ’<’, ’<=’, ’>’ and ’>=’ */
%token <node> ADDSUB            /* ’+’ et ’-’ (binairy or unary) */
%token <node> DIVSTAR           /* ’*’, ’/’ and ’%’ */
%token <node> OR
%token <node> AND

%token <node> VOID
%token <node> IF
%token <node> WHILE
%token <node> RETURN
%token <node> ELSE


%token <node> STRUCT

%type <node> Prog DeclVars DeclStructVars DeclFoncts Declarateurs DeclFonct EnTeteFonct Parametres ListTypVar Corps SuiteInstr Instr IdExpr Exp TB FB M E T F Arguments ListExp

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
       Declarateurs ',' IDENT {Node* id = makeNodeFull($3); addSibling($1, id);}
    |  IDENT {Node* cur = makeNodeFull($1); $$ = cur;}
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

        Node* type = makeNodeFull($1);
        Node* ident = makeNodeFull($2);

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
    IDENT '.' IdExpr {
     Node* cur = makeNodeFull($1);
     addChild(cur, $3);// addChild(cur, makeNodeFull($3));
     $$ = cur;
    }
    | IDENT {
     //Node* cur = makeNode(IdExpr);
     //addChild(cur, makeNodeFull($1));
     $$ = makeNodeFull($1);
    }
Exp :  Exp OR TB {Node* oper = makeNode(Or);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  TB { $$ = $1; }
    ;
TB  :  TB AND FB {Node* oper = makeNode(And);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  FB { $$ = $1; }
    ;
FB  :  FB EQ M {Node* oper = makeNode(Eq);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  M { $$ = $1; }
    ;
M   :  M ORDER E {Node* oper = makeNode(Order);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  E { $$ = $1; }
    ;
E   :  E ADDSUB T {Node* oper = makeNode(Addsub);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  T { $$ = $1; }
    ;    
T   :  T DIVSTAR F {Node* oper = makeNode(Divstar);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  F { $$ = $1; }
    ;
F   :  ADDSUB F {
        Node* cur = makeNode(Addsub);
        addChild(cur, $2);
        $$ = cur;}
    |  '!' F {Node* cur = makeNode(F); addChild(cur, $2); $$ = cur;  /* add NOT oper*/}
    |  '(' Exp ')' { $$ = $2; }
    |  NUM {$$ = makeNodeFull($1);}
    |  CHARACTER {$$ = makeNodeFull($1);}
    |  IdExpr { $$ = $1;}
    |  IDENT '(' Arguments  ')' {Node* cur = makeNodeFull($1);
        addChild(cur, $3); $$ = cur;}
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