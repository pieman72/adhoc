%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
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
	int ret = adhoc_errorNode ? adhoc_errorNode->id : 1;
	adhoc_free();
	return ret;
}

// Report error messages for parsing, validation, and generation
int yywarn(const char *str){
	fprintf(
		stderr
		,"%sWarning:%s %s\n\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;166m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,str
	);
	return adhoc_errorNode ? adhoc_errorNode->id : 1;
}

// Stop after one input file (usually stdin EOF)
int yywrap(){
	return 1;
}

// Initialize, parse, validate, generate, clean up
int main(int argc, char** argv){
	// A buffer for reporting errors
	char processResult[80];
	processResult[0] = '\0';
	adhoc_errorNode = NULL;

	// Collect timing data
	clock_t time_begin
			,time_init
			,time_parse
			,time_validate
			,time_generate
			,time_complete;
	time_begin = clock();

	// Read in configuration files, and language packs
	adhoc_init(argc, argv, processResult);
	if(strlen(processResult)) return yyerror(processResult);
	if(ADHOC_INFO_ONLY) return 0;
	time_init = clock();
	if(ADHOC_DEBUG_INFO) fprintf(stderr, "-- %s(time)%s Initialization: %s%.2f%ss --\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_init - (float)time_begin)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
	);

	// Parse the input file/stream
	FILE* outRedir;
	int parseResult;
	outRedir = stdout;
	stdout = fopen("/dev/null", "w");
	parseResult = yyparse();
	fclose(stdout);
	stdout = outRedir;
	// Clean up parse lookahead
	yylex_destroy(); // <-- WOW This was hard to find!
	if(parseResult) return yyerror("Parse failed");
	time_parse = clock();
	if(ADHOC_DEBUG_INFO) fprintf(stderr, "-- %s(time)%s Parse: %s%.2f%ss --\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_parse - (float)time_init)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
	);

	// Validate and optimize the parse tree
	adhoc_validate(processResult);
	if(strlen(processResult)) return yyerror(processResult);
	time_validate = clock();
	if(ADHOC_DEBUG_INFO) fprintf(stderr, "-- %s(time)%s Validation: %s%.2f%ss --\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_validate - (float)time_parse)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
	);

	// Generate the target translation
	adhoc_generate(processResult);
	if(strlen(processResult)) yywarn(processResult);
	time_generate = clock();
	if(ADHOC_DEBUG_INFO) fprintf(stderr, "-- %s(time)%s Code Generation: %s%.2f%ss --\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_generate - (float)time_validate)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
	);

	// Clean up and return
	int ret = adhoc_errorNode ? adhoc_errorNode->id : 0;
	adhoc_free();
	time_complete = clock();
	if(ADHOC_DEBUG_INFO) fprintf(stderr, "-- %s(time)%s Clean-up: %s%.2f%ss --\n-- %s(time)%s TOTAL: %s%.2f%ss --\n"
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_complete - (float)time_generate)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;241m" : "")
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		,(ADHOC_OUPUT_COLOR ? "[38;5;196m" : "")
		,(((float)time_complete - (float)time_begin)/1000.0F)
		,(ADHOC_OUPUT_COLOR ? "[39m" : "")
	);
	return ret;
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
				int len = strlen($<strVal>$);
				if(!strcmp($<strVal>$, "NULL")) len = 0;
				readNode->package = malloc(len+1);
				strncpy(readNode->package, ($<strVal>$), len);
				readNode->package[len] = '\0';
			};
node_name	: T_STRING {
				int len = strlen($<strVal>$);
				if(!strcmp($<strVal>$, "NULL")) len = 0;
				readNode->name = malloc(len+1);
				strncpy(readNode->name, ($<strVal>$), len);
				readNode->name[len] = '\0';
			};
node_val	: T_STRING {
				int len = strlen($<strVal>$);
				if(!strcmp($<strVal>$, "NULL")) len = 0;
				readNode->value = malloc(len+1);
				strncpy(readNode->value, ($<strVal>$), len);
				readNode->value[len] = '\0';
			};
