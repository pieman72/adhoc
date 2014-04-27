#ifndef C_H
#define C_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

// Keep track of the current scope
ASTnode* scope;

// Maintain a list of all functions that need to be declared at the top level
ASTnode** functions;
int countFuncs, sizeFuncs;

// Headers for master functions
void initialize(ASTnode*, short, FILE*, hashMap*, char*);
void generate(bool, ASTnode*, short, FILE*, hashMap*, char*);

// Assigns scope of v to s
void assignScope(ASTnode* v, ASTnode* s){
	// Set the scope pointer in v
	v->scope = s;

	// Only variable declarations need to be added to the parent
	if(v->which != ASSIGNMENT_EQUAL && v->which != VARIABLE_ASIGN) return;

	// No need to define twice
	if(v->defined) return;
	v->defined = true;

	// If s has no scopeVars, allocate the scopeVars array
	if(!s->countScopeVars){
		s->scopeVars = malloc(sizeof(ASTnode*));
		s->sizeScopeVars = 1;
	// If s is full of scopeVars, double the scopeVars array
	}else if(s->countScopeVars == s->sizeScopeVars){
		s->sizeScopeVars *= 2;
		s->scopeVars = realloc(s->scopeVars, s->sizeScopeVars*sizeof(ASTnode*));
	}
	// Add the node to its parent
	s->scopeVars[s->countScopeVars++] = v;
}

// Find the scope where a variable name v was first defined above scope s
ASTnode* findScope(char* v, ASTnode* s){
	int i;
	while(1){
		for(i=0; i<s->countScopeVars; ++i){
			if(!strcmp(v, s->scopeVars[i]->name)) return s->scopeVars[i];
		}
		if(!s->scope) return NULL;
		s = s->scope;
	}
}

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
		// Set scope to this node
		scope = n;

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
					// Set scope to this node
					scope = n;
					// Initialize arguments
					initialize(n->children[i], -1, outFile, nodes, errBuf);
				}else{
					// Set scope to this node
					scope = n;
					// Generate arguments
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

			// Close the function signature and handle children
			fprintf(outFile, ";\n");
			for(; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Initialize children
				initialize(n->children[i], indent, outFile, nodes, errBuf);

				// Type is determined by type of first returned child
				// TODO: This will hang on recursion, but I'll jump off that bridge later...
				if(n->dataType == TYPE_VOID
						&& n->children[i]->dataType != TYPE_VOID
						&& n->which == CONTROL_RETRN){
					n->dataType = n->children[i]->dataType;
				}
			}

		// Otherwise we print the full function body
		}else{
			// If it is not time to define this action, skip for now
			if(!defin) break;

			// Open the body, and print its statements
			fprintf(outFile, "{\n");
			for(; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Generate children
				generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}

			// Close the function body
			lang_c_indent(indent, outFile);
			fprintf(outFile, "}\n");
		}
		break;

	case ACTION_CALL:
		// Nothing needs to be done during initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				initialize(n->children[i], -1, outFile, nodes, errBuf);
			}
			break;
		}

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
				// Set scope to this node
				scope = n;
				// Initialize children
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "IF statements are not implemented yet :(");
		break;

	case CONTROL_LOOP:
		// Nothing to do during initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Initialize children
				initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Open for statement
		lang_c_indent(indent, outFile);
		fprintf(outFile, "for(");

		// Find the initialization and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION){
				// Set scope to this node
				scope = n;
				// Generate initialization
				generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ");

		// Find the condition and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				// Set scope to this node
				scope = n;
				// Generate condition
				generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ){\n");

		// Print all the body elements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION) continue;
			if(n->children[i]->childType == CONDITION) continue;
			// Set scope to this node
			scope = n;
			// Generate children
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the for statement
		lang_c_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_SWITCH:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Initialize children
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
				// Set scope to this node
				scope = n;
				// Initialize children
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
			if(n->which == VARIABLE_ASIGN && n->dataType == TYPE_VOID){
				n->dataType = n->children[i]->dataType;
			}
		}
		if(n->which == VARIABLE_EVAL){
			ASTnode* def;
			def = findScope(n->name, scope);
			if(def) n->dataType = def->dataType;
		}
		if(n->childType == PARAMETER){
			lang_c_printTypeName(n, outFile);
		}
	}else{
		for(i=0; i<n->countChildren; ++i){
			generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}
		if(n->childType == PARAMETER){
			lang_c_printTypeName(n, outFile);
			fprintf(outFile, " %s", n->name);
		}
	}
}

void generate_literal(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		switch(n->which){
			case LITERAL_BOOL: n->dataType = TYPE_BOOL; break;
			case LITERAL_INT: n->dataType = TYPE_INT; break;
			case LITERAL_FLOAT: n->dataType = TYPE_FLOAT; break;
			case LITERAL_STRNG: n->dataType = TYPE_STRNG; break;
			case LITERAL_ARRAY: n->dataType = TYPE_ARRAY; break;
			case LITERAL_HASH: n->dataType = TYPE_HASH; break;
			case LITERAL_STRCT: n->dataType = TYPE_STRCT; break;
		}
	}
}

void initialize(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	// Assign the scope of this node to the current scope
	// TODO: Later, this should be done during validation
	if(scope) assignScope(n, scope);

	// Handle different node types
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
