%{
#include <stdio.h>
#include <tree.h>
#include "parser.tab.h"      //tokens of syntax analyser
int linenum = 1;
int fileno(FILE *stream);

/*struct {id} {return STRUCT}*/

%}

%option noyywrap


%%
return              {return RETURN;}
while               {return WHILE;}
if                  {return IF;}
void                {return VOID;}
else                {return ELSE;}

int                 {return TYPE;}
char                {return TYPE;}

struct              {return STRUCT;}

[_a-zA-Z][_a-zA-Z0-9]* {return IDENT;}

[0-9]+              {return NUM;}
\'[^']+\'           {return CHARACTER;}

"=="|"!="           {return EQ;}
">"|">="|"<"|"<="   {return ORDER;}
[+-]                {return ADDSUB;}
[*/%]               {return DIVSTAR;}
"||"                {return OR;}
"&&"                {return AND;}

\/\/.*              {;}
[ \t\r]             {;}
\n                  {++linenum;}

.                   {return yytext[0];}
%%