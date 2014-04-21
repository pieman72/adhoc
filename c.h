#ifndef C_H
#define C_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

void lang_c_generate(ASTnode*, short, FILE*, hashMap*, char*);

void lang_c_indent(short i, FILE* o){
	if(i>=0) fprintf(o, "%.*s", i, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
}
void generate_null(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	sprintf(errBuf, "Null nodes must be removed before generating.");
}
void generate_action(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	switch(n->which){
	case ACTION_DEFIN:
		// Print the function return type and name
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s %s(", "void", n->name);

		// Print the action's parameters
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == PARAMETER){
				if(i>0) fprintf(outFile, ", ");
				lang_c_generate(n->children[i], -1, outFile, nodes, errBuf);
			}else{
				break;
			}
		}

		// Close the function signature line
		fprintf(outFile, "){\n");

		// Handle the body of the function
		for(; i<n->countChildren; ++i){
			lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the function itself
		lang_c_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case ACTION_CALL:
		// Indent this function call, and call it
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s(", n->name);

		// Print the action's arguments
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == ARGUMENT){
				if(i>0) fprintf(outFile, ", ");
				lang_c_generate(n->children[i], -1, outFile, nodes, errBuf);
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
void generate_group(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}
void generate_control(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}
void generate_operator(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}
void generate_assignment(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}
void generate_variable(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}
void generate_literal(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		lang_c_generate(n->children[i], indent+1, outFile, nodes, errBuf);
	}
}

void lang_c_generate(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	switch(n->nodeType){
		case TYPE_NULL: generate_null(n, indent, outFile, nodes, errBuf); break;
		case ACTION: generate_action(n, indent, outFile, nodes, errBuf); break;
		case GROUP: generate_group(n, indent, outFile, nodes, errBuf); break;
		case CONTROL: generate_control(n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: generate_operator(n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: generate_assignment(n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: generate_variable(n, indent, outFile, nodes, errBuf); break;
		case LITERAL: generate_literal(n, indent, outFile, nodes, errBuf); break;
	}
}

#endif
