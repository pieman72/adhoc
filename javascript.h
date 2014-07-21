#ifndef ADHOC_JAVASCRIPT_H
#define ADHOC_JAVASCRIPT_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

// Keep track of the current scope
ASTnode* scope;

// Maintain a list of all functions that need to be declared at the top level
ASTnode** functions;
int countFuncs, sizeFuncs;
bool isExec;

// Headers for master functions
void lang_javascript_initialize(ASTnode*, short, FILE*, hashMap*, char*);
void lang_javascript_generate(bool, ASTnode*, short, FILE*, hashMap*, char*);

// Find the scope where a variable name v was first defined above scope s
ASTnode* lang_javascript_findScope(char* v, ASTnode* s){
	int i;
	while(1){
		for(i=0; i<s->countScopeVars; ++i){
			if(!strcmp(v, s->scopeVars[i]->name)) return s->scopeVars[i];
		}
		if(!s->scope) return NULL;
		s = s->scope;
	}
}

// Assigns scope of v to s
void lang_javascript_assignScope(ASTnode* v, ASTnode* s){
	// See if the scope was already set
	ASTnode* def = lang_javascript_findScope(v->name, s);
	if(def){
		v->scope = def->scope;
		return;
	}

	// Set the scope pointer in v
	v->scope = s;

	// Only variable declarations need to be added to the parent
	if(v->which != VARIABLE_ASIGN) return;

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

// C names for data types
void lang_javascript_printTypeName(ASTnode* n, FILE* o){
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
void lang_javascript_indent(short i, FILE* o){
	if(i>=0) fprintf(o, "%.*s", i, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
}

// Generating Null nodes should just throw an error
void lang_javascript_generate_null(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	adhoc_errorNode = n->parent;
	sprintf(errBuf, "Null nodes should be removed before generating.");
}

// Generating actions differs most between init and gen, and decl and call
void lang_javascript_generate_action(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i,j;
	switch(n->which){
	case ACTION_DEFIN:

		// For the init run, nothing is printed
		if(isInit){
			// If this is a top-level action, add it to the list of functions
			if(!n->scope){
				if(++countFuncs > sizeFuncs){
					sizeFuncs *= 2;
					functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
				}
				functions[countFuncs-1] = n;
			}

			// Handle the children
			for(i=0; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;

		// Print the full function body
		}else{
			// Leave a comment above the function definition
			fprintf(outFile, "\n");
			lang_javascript_indent(indent, outFile);
			if(n->value && strlen(n->value)) fprintf(outFile, "// %s\n", n->value);

			// If a statment, print "function" keyword and name
			if(n->childType == STATEMENT || n->childType == CHILD_NULL){
				lang_javascript_indent(indent, outFile);
				if(isExec){
					fprintf(outFile, "%s.%s = function(", n->package, n->name);
				}else{
					fprintf(outFile, "function %s(", n->name);
				}
			// If an expression, just print the keyword
			}else if(n->childType == EXPRESSION){
				fprintf(outFile, "function(");
			}

			// Print the action's parameters
			for(i=0; i<n->countChildren; ++i){
				if(n->children[i]->childType == PARAMETER){
					if(i>0) fprintf(outFile, ", ");
					if(isInit){
						// Set scope to this node
						scope = n;
						// Initialize arguments
						lang_javascript_initialize(n->children[i], -1, outFile, nodes, errBuf);
					}else{
						// Set scope to this node
						scope = n;
						// Generate arguments
						lang_javascript_generate(false, n->children[i], -1, outFile, nodes, errBuf);
					}
				}else{
					break;
				}
			}

			// Close the function signature and open the body block
			fprintf(outFile, "){\n");

			// Print declarations for any scope vars
			for(j=0; j<n->countScopeVars; ++j){
				if(n->scopeVars[j]->childType == PARAMETER) continue;
				lang_javascript_indent(indent+1, outFile);
				fprintf(outFile, "var %s;\n", n->scopeVars[j]->name);
			}

			// Print the child statements
			for(; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Generate children
				lang_javascript_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}

			// Close the function body
			lang_javascript_indent(indent, outFile);
			fprintf(outFile, "}\n");
			if(!n->parent) break;
		}

	case ACTION_CALL:
		// Nothing needs to be done during initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				lang_javascript_initialize(n->children[i], -1, outFile, nodes, errBuf);
			}
			break;
		}

		// Indent this function call, and call it
		lang_javascript_indent(indent, outFile);
		if(isExec && strcmp(n->package, "System")){
			fprintf(outFile, "%s.%s(", n->package, n->name);
		}else{
			fprintf(outFile, "%s(", n->name);
		}

		// Print the action's arguments
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == ARGUMENT || n->children[i]->childType == PARAMETER){
				if(i>0) fprintf(outFile, ", ");
				lang_javascript_generate(true, n->children[i], -1, outFile, nodes, errBuf);
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
void lang_javascript_generate_group(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		// Initialize all children in order
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		// Generate all children in order
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_generate(false, n->children[i], indent, outFile, nodes, errBuf);
		}
	}
}

// Controls vary greatly
void lang_javascript_generate_control(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	switch(n->which){
	case CONTROL_IF:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Print the opening of the 'if' block
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "if(");

		// Print the condition
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				lang_javascript_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}

		// Close the 'if' line
		fprintf(outFile, "){\n");

		// Print the 'if' statements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType != IF) continue;
			lang_javascript_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Print the 'else' statements
		bool needElse = true;
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType != ELSE) continue;
			if(needElse){
				lang_javascript_indent(indent, outFile);
				fprintf(outFile, "}else{\n");
				needElse = false;
			}
			lang_javascript_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the whole 'if' block
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_LOOP:
		// Nothing to do during initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Declare scope vars
		for(i=0; i<n->countScopeVars; ++i){
			lang_javascript_indent(indent, outFile);
			fprintf(outFile, "var %s;\n", n->scopeVars[i]->name);
		}

		// Open for statement
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "for(");

		// Find the initialization and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION){
				// Generate initialization
				lang_javascript_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ");

		// Find the condition and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				// Generate condition
				lang_javascript_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ){\n");

		// Print all the body elements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION) continue;
			if(n->children[i]->childType == CONDITION) continue;
			// Generate children
			lang_javascript_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the for statement
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_SWITCH:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "SWITCH statements are not implemented yet :(");
		break;

	case CONTROL_CASE:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "CASE statements are not implemented yet :(");
		break;

	case CONTROL_FORK:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "FORK statements are not implemented yet :(");
		break;

	case CONTROL_CNTNU:
	case CONTROL_BREAK:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Just print the keyword and a semicolon
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "%s;\n", adhoc_nodeWhich_names[n->which]);
		break;

	case CONTROL_RETRN:
		// During initialization, set the parent function's datatype
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);

				// Get datatype from child and pass it to the parent function
				n->dataType = n->children[i]->dataType;
				if(scope){
					if(scope->dataType==TYPE_VOID && n->dataType!=TYPE_VOID){
						scope->dataType = n->dataType;
					}
				}
			}
			break;
		}

		// Print the keyword
		lang_javascript_indent(indent, outFile);
		fprintf(outFile, "return ");

		// Generate the children
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_generate(false, n->children[i], 0, outFile, nodes, errBuf);
		}
		fprintf(outFile, ";\n");
		break;
	}
}

// Generation rules for operators
void lang_javascript_generate_operator(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			switch(n->which){
				case OPERATOR_NOT:
					n->dataType = TYPE_BOOL;
					break;
				case OPERATOR_TRNIF:
					n->dataType = n->children[1]->dataType;
					break;
				default:
					n->dataType = n->children[0]->dataType;
			}
		}
	}else{
		if(n->childType == STATEMENT) lang_javascript_indent(indent, outFile);
		switch(n->which){
			case OPERATOR_NOT:
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_ARIND:
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, "[");
				lang_javascript_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, "]");
				break;
			case OPERATOR_PLUS:
			case OPERATOR_MINUS:
			case OPERATOR_TIMES:
			case OPERATOR_DIVBY:
			case OPERATOR_MOD:
			case OPERATOR_EXP:
			case OPERATOR_OR:
			case OPERATOR_AND:
			case OPERATOR_EQUIV:
			case OPERATOR_GRTTN:
			case OPERATOR_LESTN:
			case OPERATOR_GRTEQ:
			case OPERATOR_LESEQ:
			case OPERATOR_NOTEQ:
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				lang_javascript_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_TRNIF:
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " ? ");
				lang_javascript_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " : ");
				lang_javascript_generate(false, n->children[2], indent+1, outFile, nodes, errBuf);
				break;
		}
		if(n->childType == STATEMENT) fprintf(outFile, ";\n");
	}
}

// Generation rules for assignments
void lang_javascript_generate_assignment(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		// Initialize the children and pass their types to the assignment and storage
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
		switch(n->which){
			// Boolean types
			case ASSIGNMENT_OR:
			case ASSIGNMENT_AND:
			case ASSIGNMENT_NEGPR:
			case ASSIGNMENT_NEGPS:
				n->dataType = TYPE_BOOL;
				n->children[0]->dataType = TYPE_BOOL;
				break;
			// Unaries
			case ASSIGNMENT_INCPR:
			case ASSIGNMENT_INCPS:
			case ASSIGNMENT_DECPR:
			case ASSIGNMENT_DECPS:
				n->dataType = n->children[0]->dataType;
				break;
			// Other assignments take their types from second argument
			default:
				n->dataType = n->children[1]->dataType;
				n->children[0]->dataType = n->dataType;
		}
	}else{
		if(n->childType == STATEMENT
				|| n->childType == IF
				|| n->childType == ELSE)
			lang_javascript_indent(indent, outFile);
		switch(n->which){
			case ASSIGNMENT_INCPR:
			case ASSIGNMENT_DECPR:
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				break;
			case ASSIGNMENT_INCPS:
			case ASSIGNMENT_DECPS:
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				break;
			case ASSIGNMENT_EQUAL:
			case ASSIGNMENT_PLUS:
			case ASSIGNMENT_MINUS:
			case ASSIGNMENT_TIMES:
			case ASSIGNMENT_DIVBY:
			case ASSIGNMENT_MOD:
			case ASSIGNMENT_EXP:
			case ASSIGNMENT_OR:
			case ASSIGNMENT_AND:
				lang_javascript_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				lang_javascript_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				break;
			case ASSIGNMENT_NEGPR:
			case ASSIGNMENT_NEGPS:
				sprintf(
					errBuf
					,"Node %d: %s"
					,n->children[0]->id
					,"Negation operator does not exist in C."
				);
				break;
		}
		if(n->childType == STATEMENT
				|| n->childType == IF
				|| n->childType == ELSE)
			fprintf(outFile, ";\n");
	}
}

// Generation rules for variables
void lang_javascript_generate_variable(bool isInit, bool call, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_javascript_initialize(n->children[i], indent, outFile, nodes, errBuf);
			if(n->which == VARIABLE_ASIGN && n->dataType == TYPE_VOID){
				n->dataType = n->children[i]->dataType;
			}
		}
		if(n->which == VARIABLE_EVAL){
			ASTnode* def;
			def = lang_javascript_findScope(n->name, scope);
			if(def) n->dataType = def->dataType;
		}
	}else{
		if(n->childType == INITIALIZATION){
			fprintf(outFile, "%s = ", n->name);
		}else if(!(n->childType==PARAMETER && call)){
			fprintf(outFile, "%s", n->name);
		}
		if(n->childType!=PARAMETER || call){
			for(i=0; i<n->countChildren; ++i){
				lang_javascript_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}
		}
	}
}

// Generation rules for literals
void lang_javascript_generate_literal(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
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
	}else{
		switch(n->which){
			case LITERAL_BOOL:
				if(!strcmp(n->value, "true") || !strcmp(n->value, "false")){
					fprintf(outFile, "%s", n->value);
				}else{
					fprintf(outFile, "%s", (atoi(n->value) ? "true" : "false"));
				}
				break;
			case LITERAL_INT:
				fprintf(outFile, "%d", atoi(n->value));
				break;
			case LITERAL_FLOAT:
				fprintf(outFile, "%s", n->value);
				break;
			case LITERAL_STRNG:
				fprintf(outFile, "\"%s\"", n->value);
				break;
			case LITERAL_ARRAY:
				sprintf(errBuf, "%s", "Printing arrays is not implemented :(");
				break;
			case LITERAL_HASH:
				sprintf(errBuf, "%s", "Printing hashes is not implemented :(");
				break;
			case LITERAL_STRCT:
				sprintf(errBuf, "%s", "Printing structs is not implemented :(");
				break;
		}
	}
}

// Function to initialize an AST node
void lang_javascript_initialize(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	// Assign the scope of this node to the current scope
	// TODO: Later, this should be done during validation
	if(scope) lang_javascript_assignScope(n, scope);

	// Handle different node types
	switch(n->nodeType){
		case TYPE_NULL: lang_javascript_generate_null(true, n, indent, outFile, nodes, errBuf); break;
		case ACTION: lang_javascript_generate_action(true, n, indent, outFile, nodes, errBuf); break;
		case GROUP: lang_javascript_generate_group(true, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: lang_javascript_generate_control(true, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: lang_javascript_generate_operator(true, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: lang_javascript_generate_assignment(true, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: lang_javascript_generate_variable(true, false, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: lang_javascript_generate_literal(true, n, indent, outFile, nodes, errBuf); break;
	}
}
// Function to generate code from an AST node
void lang_javascript_generate(bool call, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	switch(n->nodeType){
		case TYPE_NULL: lang_javascript_generate_null(false, n, indent, outFile, nodes, errBuf); break;
		case ACTION: lang_javascript_generate_action(false, n, indent, outFile, nodes, errBuf); break;
		case GROUP: lang_javascript_generate_group(false, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: lang_javascript_generate_control(false, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: lang_javascript_generate_operator(false, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: lang_javascript_generate_assignment(false, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: lang_javascript_generate_variable(false, call, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: lang_javascript_generate_literal(false, n, indent, outFile, nodes, errBuf); break;
	}
}

// Hook function for generalized initialization
void lang_javascript_init(ASTnode* n, FILE* outFile, hashMap* nodes, bool exec, char* errBuf){
	isExec = exec;
	countFuncs = 0;
	sizeFuncs = 2;
	functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
	if(exec){
		//fprintf(outFile, "#include <libadhoc.h>\n");
	}
	lang_javascript_initialize(n, 0, outFile, nodes, errBuf);
}
// Hook function for generalized code generation
void lang_javascript_gen(ASTnode* n, FILE* outFile, hashMap* nodes, bool exec, char* errBuf){
	if(exec && countFuncs){
		fprintf(outFile, "\n// Namespacing wrapper\n");
		fprintf(outFile, "var %s = %s || {}\n", functions[0]->package, functions[0]->package);
	}
	lang_javascript_generate(false, functions[0], 0, outFile, nodes, errBuf);
	if(exec && countFuncs){
		fprintf(outFile, "\n// Execute primary function from namespace\n");
		fprintf(outFile, "%s.%s();\n", functions[0]->package, functions[0]->name);
	}
	free(functions);
}

#endif
