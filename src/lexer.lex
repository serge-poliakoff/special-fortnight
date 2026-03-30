%{
#include <stdio.h>

#include <tree.h>
#include "parser.tab.h"      //tokens of syntax analyser
int linenum = 1;
int fileno(FILE *stream);

/*struct {id} {return STRUCT}*/

char* strdup(const char* source){
	char *res = (char*)malloc(strlen(source) + 1);
	strcpy(res, source);
	res[strlen(source)] = '\0';
	return res;
}

%}

%option noyywrap


%%
return              {return RETURN;}
while               {return WHILE;}
if                  {return IF;}
void                {return VOID;}
else                {return ELSE;}


int                 { tree_label int_label;
						int_label.type = TP; int_label.value.id = strdup("int");
						yylval.value = int_label; return TYPE; }
char                { tree_label int_label;
						int_label.type = TP;
						int_label.value.id = strdup("char");
						yylval.value = int_label; return TYPE; }

struct              {
						//yylval.value.type = KEYWORD; yylval.value.value.label = Struct;
						return STRUCT; }

[_a-zA-Z][_a-zA-Z0-9]* {
	
	yylval.value.type = ID;
	//printf("%s %d", yytext, strlen(yytext));
	//yylval.value.value.id = (char *)malloc(strlen(yytext) + 1);
	//yylval.value.value.id[strlen(yytext)] = '\0';
	//printf("%s %d %s", yytext, strlen(yytext), yylval.value.value.id);
	//assert(yylval.value.value.id != NULL);
	//strcpy(yylval.value.value.id, yytext);
	yylval.value.value.id = strdup(yytext);
	return IDENT;
}

[0-9]+ {
	yylval.value.type = INT;
	yylval.value.value.number = atoi(yytext);
	return NUM;
}
\'[^']+\' {
	yylval.value.type = CHAR;
	yylval.value.value.character = yytext[1];
	return CHARACTER;
}

"=="|"!="           {
	yylval.value.type = OP;
	yylval.value.value.id = strdup(yytext);
	return EQ;}
">"|">="|"<"|"<="   {
	yylval.value.type = OP;
	yylval.value.value.id = strdup(yytext);
	return ORDER;}
[+-]                {
	yylval.value.type = OP;
	yylval.value.value.id = strdup(yytext);
	return ADDSUB;}
[*/%]               {
	yylval.value.type = OP;
	yylval.value.value.id = strdup(yytext);
	return DIVSTAR;}
"||"                {return OR;}
"&&"                {return AND;}

\/\/.*              {;}
[ \t\r]             {;}
\n                  {++linenum;}

.                   {return yytext[0];}
%%