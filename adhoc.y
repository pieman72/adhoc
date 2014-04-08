%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc.h"
 
// Report error messages for parsing, validation, and generation
int yyerror(const char *str){
	fprintf(
		stderr
		,"%sError:%s %s\n\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;160m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,str
	);
	adhoc_free();
	return 1;
}

// Stop after one input file (usually stdin EOF)
int yywrap(){
	return 1;
} 

// Initialize, parse, validate, generate, clean up
main(int argc, char** argv){
	char* processResult;

	// Read in configuration files, and language packs
	processResult = adhoc_init(argc, argv);
	if(processResult) return yyerror(processResult);

	// Parse the input file/stream
	yyparse();
	yylex_destroy(); // <-- WOW This was hard to find!

	// Validate and optimize the parse tree
	processResult = adhoc_validate();
	if(processResult) return yyerror(processResult);

	// Generate the target translation
	processResult = adhoc_generate();
	if(processResult) return yyerror(processResult);

	// Clean up
	return adhoc_free();
} 
%}

// These are used to return int and string values from the parse
%union {
	int intVal;
	char* strVal;
}

// These are the acceptable terminal tokens during a parse
%token T_3BYTE T_STRING

// Begin parse rules
%%


nodes		: node nodes {
			}
			| ;

node		: node_id parent_id node_type which child_type
			node_pkg node_name node_val {
				adhoc_insertNode(readNode);
				readNode = adhoc_createBlankNode();
			};
node_id		: T_3BYTE {
				readNode->id = $<intVal>$;
			};
parent_id	: T_3BYTE {
				readNode->parentId = $<intVal>$;
			};
node_type	: T_3BYTE {
				readNode->nodeType = $<intVal>$;
			};
which		: T_3BYTE {
				readNode->which = $<intVal>$;
			};
child_type	: T_3BYTE {
				readNode->childType = $<intVal>$;
			};
node_pkg	: T_STRING {
				int len = strlen($<strVal>$)-2;
				readNode->package = malloc(len+1);
				strncpy(readNode->package, ($<strVal>$+1), len);
				readNode->package[len] = '\0';
			};
node_name	: T_STRING {
				int len = strlen($<strVal>$)-2;
				readNode->name = malloc(len+1);
				strncpy(readNode->name, ($<strVal>$+1), len);
				readNode->name[len] = '\0';
			};
node_val	: T_STRING {
				int len = strlen($<strVal>$)-2;
				readNode->value = malloc(len+1);
				strncpy(readNode->value, ($<strVal>$+1), len);
				readNode->value[len] = '\0';
			};
