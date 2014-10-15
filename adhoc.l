%{
#include <stdio.h>
#include <string.h>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#include "y.tab.h"

extern YYSTYPE yylval;
char *s, buf[1024];
%}

%x STRING

%%

\"						{
						s = buf;
						BEGIN(STRING);
						}

<STRING>\\.				{
						*s++ = yytext[0];
						*s++ = yytext[1];
						}

<STRING>\"				{
						BEGIN(INITIAL);
						*s = '\0';
						yylval.strVal = buf; return T_STRING;
						}

<STRING>[^\\\"]+		{
						s += strlen(strcpy(s, yytext));
						}

(?s:[^"]..)				{ /* This regex is jank, and will break eventually */
						yylval.intVal = ((yytext[0]*256)+yytext[1])*256+yytext[2];
						return T_3BYTE;
						}

%%
#pragma clang diagnostic pop
