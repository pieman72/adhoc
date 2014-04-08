%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"
extern YYSTYPE yylval;
%}

%%

\"([^\"]|\\\")*\"	yylval.strVal = yytext; return T_STRING;
...					yylval.intVal = ((yytext[0]*256)+yytext[1]*256)+yytext[2]; return T_3BYTE;

%%
