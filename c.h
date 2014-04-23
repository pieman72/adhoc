#ifndef C_H
#define C_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

ASTnode** functions;
int countFuncs, sizeFuncs;

// Headers for master functions
void initialize(ASTnode*, short, FILE*, hashMap*, char*);
void generate(bool, ASTnode*, short, FILE*, hashMap*, char*);

// C names for data types
char* lang_c_printTypeName(ASTnode* n, FILE* o){
	switch(n->dataType){
	case TYPE_NULL:
		fprintf(o, "void");
		break;
	case TYPE_BOOL:
		fprintf(o, "bool");
		break;
	case TYPE_INT:
		fprintf(o, "int");
		break;
	case TYPE_FLOAT:
		fprintf(o, "float");
		break;
	case TYPE_STRNG:
		fprintf(o, "char*");
		break;
	case TYPE_ARRAY:
		fprintf(o, "<<ARRAY>>");
		break;
	case TYPE_HASH:
		fprintf(o, "<<HASH>>");
		break;
	case TYPE_STRCT:
		fprintf(o, "<<STRUCT>>");
		break;
	case TYPE_ACTN:
		fprintf(o, "<<ACTION>>");
		break;
	}
}

// Indentation function
void lang_c_indent(short i, FILE* o){
	if(i>=0) fprintf(o, "%.*s", i, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
}

// Generating Null nodes should just throw an error
void generate_null(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	sprintf(errBuf, "Null nodes must be removed before generating.");
}

// Generating actions differs most between init and gen, and decl and call
void generate_action(bool isInit, bool defin, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(!isInit){
		if(defin) n->which = ACTION_DEFIN;
		else n->which = ACTION_CALL;
	}
	switch(n->which){
	case ACTION_DEFIN:
		// Leave a comment above the function definition
		fprintf(outFile, "\n");
		lang_c_indent(indent, outFile);
		fprintf(outFile, "// %s\n", n->value);

		// Print the function return type and name
		lang_c_indent(indent, outFile);
		lang_c_printTypeName(n, outFile);
		fprintf(outFile, " %s(", n->name);

		// Print the action's parameters
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == PARAMETER){
				if(i>0) fprintf(outFile, ", ");
				if(isInit){
					initialize(n->children[i], -1, outFile, nodes, errBuf);
				}else{
					generate(false, n->children[i], -1, outFile, nodes, errBuf);
				}
			}else{
				break;
			}
		}

		// Close the function signature
		fprintf(outFile, ")");

		// If this is just a signature, print it and check the children
		if(isInit){
			// First add it to the list of functions
			if(++countFuncs > sizeFuncs){
				sizeFuncs *= 2;
				functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
			}
			functions[countFuncs-1] = n;

			fprintf(outFile, ";\n");
			for(; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}

		// Otherwise we print the full function body
		}else{
			// If it is not time to define this action, skip for now
			if(!defin) break;

			// Open the body, and print its statements
			fprintf(outFile, "{\n");
			for(; i<n->countChildren; ++i){
				generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}

			// Close the function body
			lang_c_indent(indent, outFile);
			fprintf(outFile, "}\n");
		}
		break;

	case ACTION_CALL:
		// Nothing needs to be done during initialization
		if(isInit) break;

		// Indent this function call, and call it
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s(", n->name);

		// Print the action's arguments
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == ARGUMENT){
				if(i>0) fprintf(outFile, ", ");
				generate(false, n->children[i], -1, outFile, nodes, errBuf);
			}
		}

		// Close the function call
		fprintf(outFile, ")");

		// If this is the end of a statement, add a semicolon
		if(n->childType == STATEMENT){
			fprintf(outFile, ";\n");
		}
		break;
	}
}

// Groups are just a sequential ordering of children, nothing need be done
void generate_group(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		// Initialize all children in order
		for(i=0; i<n->countChildren; ++i){
			initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		// Generate all children in order
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent, outFile, nodes, errBuf);
		}
	}
}

// Controls vary greatly
void generate_control(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	switch(n->which){
	case CONTROL_IF:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "IF statements are not implemented yet :(");
		break;

	case CONTROL_LOOP:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		lang_c_indent(indent, outFile);
		fprintf(outFile, "for(");
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION){
				generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ");
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ){\n");
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION) continue;
			if(n->children[i]->childType == CONDITION) continue;
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
		lang_c_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_SWITCH:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "SWITCH statements are not implemented yet :(");
		break;

	case CONTROL_CASE:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "CASE statements are not implemented yet :(");
		break;

	case CONTROL_FORK:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "FORK statements are not implemented yet :(");
		break;
	}
}

void generate_operator(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
	}
}

void generate_assignment(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
	}
}

void generate_variable(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
	}
}

void generate_literal(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
	}
}

void initialize(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	switch(n->nodeType){
		case TYPE_NULL: generate_null(true, n, indent, outFile, nodes, errBuf); break;
		case ACTION: generate_action(true, false, n, indent, outFile, nodes, errBuf); break;
		case GROUP: generate_group(true, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: generate_control(true, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: generate_operator(true, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: generate_assignment(true, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: generate_variable(true, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: generate_literal(true, n, indent, outFile, nodes, errBuf); break;
	}
}
void generate(bool defin, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	switch(n->nodeType){
		case TYPE_NULL: generate_null(false, n, indent, outFile, nodes, errBuf); break;
		case ACTION: generate_action(false, defin, n, indent, outFile, nodes, errBuf); break;
		case GROUP: generate_group(false, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: generate_control(false, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: generate_operator(false, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: generate_assignment(false, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: generate_variable(false, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: generate_literal(false, n, indent, outFile, nodes, errBuf); break;
	}
}

void lang_c_init(ASTnode* n, FILE* outFile, hashMap* nodes, char* errBuf){
	countFuncs = 0;
	sizeFuncs = 2;
	functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
	initialize(n, 0, outFile, nodes, errBuf);
}
void lang_c_gen(ASTnode* n, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<countFuncs; ++i){
		generate(true, functions[i], 0, outFile, nodes, errBuf);
	}
	free(functions);
}

#endif
