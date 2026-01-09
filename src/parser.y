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

%define api.value.type {Node*}
%token ID
%token DIVSTAR
%%
E: T {expr = makeNode(E);
	addChild(expr, $1); };
T: T DIVSTAR ID	{
		Node* curr = makeNode(T);	//current node, which we will return
		Node* ops = makeNode(divstar);
		Node* identity = makeNode(id);
		addChild(curr, $1);
		addSibling($1, ops);
		addSibling(ops, identity);
		$$ = curr; 
	}
    | ID {
		Node* curr = makeNode(T);
		addChild(curr, makeNode(id));
		$$ = curr; };
%%

extern int linenum;
void yyerror(char* s){
    fprintf(stderr, "Syntax error on line %d: %s\n", linenum, s);
	deleteTree(expr);
	exit(1);
}

int main() {
	yyin = stdin;

	do {
		yyparse();
	} while(!feof(yyin));

	printTree(expr);
	deleteTree(expr);
	return 0;
}