%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tree.h>
#include <semantics.h>
#include <compiler.h>
#include <vartable.h>
extern int yylex();
extern int yyparse();
extern FILE* yyin;

void yyerror(const char* s);
Node* prog;

//TODO:
// - make node for NOT bool operator

//the func below is not to be used - for now stick to IDENT names for structure types - 
// anyway because of lexer you can't define "struct int" or smth like this,
// so it is allover much easier to refer struct types just by their name
// struct keyword would be so present just in the declaration of the type

// modifies IDENT in "STRUCT IDENT" pattern to be "struct IDENT"
// in a structure typed variable declaration
void create_struct_type_name(tree_label *identity){
    char *struct_type_id = identity->value.id;
    char *full_struct_type_name = 
        (char *)malloc(8 + strlen(struct_type_id)); // 'struct ' + \0
    strcat(strcat(full_struct_type_name, "struct "), struct_type_id);
    full_struct_type_name[7 + strlen(struct_type_id)] = '\0';
    identity->value.id = full_struct_type_name;
    free(struct_type_id);
}

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
%token <value> EQ                /* egality (’==’) and inegality (’!=’) operators */
%token <value> ORDER             /* comparaison operators ’<’, ’<=’, ’>’ and ’>=’ */
%token <value> ADDSUB            /* ’+’ et ’-’ (binairy or unary) */
%token <value> DIVSTAR           /* ’*’, ’/’ and ’%’ */
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
       DeclVars TYPE Declarateurs ';' {Node *type = makeNodeFull($2);
        addChild($1, type); addChild(type, $3); $$ = $1;}
    |  DeclVars STRUCT IDENT '{' DeclStructVars '}' ';' {
        Node *type = makeNode(Struct); $3.type = TP; Node *typeName = makeNodeFull($3);
        addChild(type, typeName);
        addChild(typeName, $5);
        addChild($1, type);
        $$ = $1;
    }
    |  DeclVars STRUCT IDENT Declarateurs ';' {
        $3.type = TP;
        Node *type = makeNodeFull($3); addChild($1, type);
        addChild(type, $4);
        $$ = $1;}
    |  {Node* cur = makeNode(DeclVars); $$ = cur;}
    ;
DeclStructVars:
       DeclStructVars TYPE Declarateurs ';' {Node *type = makeNodeFull($2);
        addSibling($1, type); addChild(type, $3); $$ = $1;}
    |  DeclStructVars STRUCT IDENT Declarateurs ';' {
        $3.type = TP; Node *typeName = makeNodeFull($3);
        addSibling($1, typeName); addChild(typeName, $4);
        $$ = $1;}
    |  TYPE Declarateurs ';' {Node *type = makeNodeFull($1);
        addChild(type, $2); $$ = type;}
    | STRUCT IDENT Declarateurs ';' {
        $2.type = TP;
        Node *typeName = makeNodeFull($2);
        addChild(typeName, $3);
        $$ = typeName;}
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
        Node* ident = makeNodeFull($2);
        addChild(cur, type); addChild(cur, ident); addChild(cur, $4);
        $$ = cur;
       }
    |  STRUCT IDENT IDENT '(' Parametres ')'{
        Node* cur = makeNode(EnTeteFonct);
        $2.type = TP;
        Node* typeName = makeNodeFull($2);
        Node* name = makeNodeFull($3);
        addChild(cur, typeName); addChild(cur, name); addChild(cur, $5);
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
        Node* type = makeNodeFull($3);
        Node* ident = makeNodeFull($4);
        addSibling($1, type); addChild(type, ident);
        $$ = $1;
        }
    |  ListTypVar ',' STRUCT IDENT IDENT {
        $4.type = TP;
        Node* typeName = makeNodeFull($4);
        Node* ident = makeNodeFull($5);
        addSibling($1, typeName); addChild(typeName, ident);
        $$ = $1;
       }
    |  STRUCT IDENT IDENT {
        $2.type = TP;
        Node* typeName = makeNodeFull($2);
        Node* ident = makeNodeFull($3);
        addChild(typeName, ident);
        $$ = typeName; 
       }
    |  TYPE IDENT {
        Node* type = makeNodeFull($1);
        Node* ident = makeNodeFull($2);
        addChild(type, ident);
        $$ = type;
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
        addChild(cur, ifT); addChild(ifT, $3); addChild(ifT, $5);
        $$ = cur;
    }
    |  IF '(' Exp ')' Instr ELSE Instr {
        Node* cur = makeNode(Instr);
        Node* ifT = makeNode(If);
        Node* elseT = makeNode(Else);
        addChild(cur, ifT); addChild(ifT, $3); addChild(ifT, $5);
        addChild(cur, elseT); addChild(elseT, $7);
        $$ = cur;
    }
    |  WHILE '(' Exp ')' Instr {
        Node* cur = makeNode(Instr);
        Node* whileT = makeNode(While);
        addChild(cur, whileT); addChild(whileT, $3); addChild(whileT, $5);
        $$ = cur;
    }
    |  IDENT '(' Arguments  ')' ';' {
        Node* cur = makeNode(Instr);
        Node* func = makeNodeFull($1);
        addChild(cur, func); addChild(func, $3);
        $$ = cur;
    }
    |  RETURN Exp ';' {
        Node* cur = makeNode(Instr);
        Node* ret = makeNode(Return);
        addChild(cur, ret); addChild(ret, $2);
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
FB  :  FB EQ M {Node* oper = makeNodeFull($2);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  M { $$ = $1; }
    ;
M   :  M ORDER E {Node* oper = makeNodeFull($2);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  E { $$ = $1; }
    ;
E   :  E ADDSUB T {Node* oper = makeNodeFull($2);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  T { $$ = $1; }
    ;    
T   :  T DIVSTAR F {Node* oper = makeNodeFull($2);
        addChild(oper, $1); addChild(oper, $3);
        $$ = oper;}
    |  F { $$ = $1; }
    ;
F   :  ADDSUB F {
        Node* cur = makeNodeFull($1);   //unary + or -
        addChild(cur, $2);
        $$ = cur;}
    |  '!' F {Node* cur = makeNode(Not); addChild(cur, $2); $$ = cur;  /* add NOT oper*/}
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
        //addChild($1, $3);
        addSibling($1, $3);
        $$ = $1;
    }
    |  Exp {
        //Node *cur = makeNode(ListExp);
        //addChild(cur, $1);
        $$ = $1;//cur;
    }
    ;
%%

extern int linenum;
void yyerror(const char* s){
    fprintf(stderr, "Syntax error on line %d: %s\n", linenum, s);
	//deleteTree(prog);
	exit(1);
}
 
int main(int argc, char **argv) {
	yyin = stdin;

	do {
		yyparse();
	} while(!feof(yyin));

    int dry_run = argc > 1 && strcmp(argv[1], "--dry-run") == 0;

    if (!dry_run){
        printTree(prog);
    }
    analyse_semantics(prog);

    if (!dry_run){
        compile(prog);
    }else{
        freeVarTables();
    }

	deleteTree(prog);
	printf("Parsed successfully\n");
    
	return 0;
}