%{
#include <stdio.h>
#include <tree.h>
#include "parser.tab.h"      //tokens of syntax analyser
int linenum = 1;
int fileno(FILE *stream);
%}

%option noyywrap


%%
[_a-zA-Z][_a-zA-Z0-9]+ return ID;
[*/] return DIVSTAR;
[ \t\r] ;
\n ++linenum;
. return yytext[0];
%%