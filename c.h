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
void lang_c_initialize(ASTnode*, short, FILE*, hashMap*, char*);
void lang_c_generate(bool, ASTnode*, short, FILE*, hashMap*, char*);

// C names for data types
void lang_c_printTypeName(ASTnode* n, FILE* o){
	int i;
	if(n->which == ACTION_DEFIN){
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->which != CONTROL_RETRN) continue;
			lang_c_printTypeName(n->children[i], o);
			return;
		}
	}
	if(n->which == CONTROL_RETRN){
		lang_c_printTypeName(n->children[0], o);
		return;
	}
	if(n->dataType == TYPE_STRNG){
		fprintf(o, "adhoc_data*");
		return;
	}
	if(n->dataType == TYPE_ARRAY){
		lang_c_printTypeName(n->children[0]->children[0], o);
		fprintf(o, "*");
		return;
	}
	if(n->dataType == TYPE_HASH){
		fprintf(o, "hashMap*");
		return;
	}
	fprintf(o, "%s", adhoc_dataType_names[n->dataType]);
}

// Indentation function
void lang_c_indent(short i, FILE* o){
	if(i>=0) fprintf(o, "%.*s", i, "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
}

// Generating Null nodes should just throw an error
void lang_c_generate_null(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	adhoc_errorNode = n->parent;
	sprintf(errBuf, "Null nodes should be removed before generating.");
}

// Generating actions differs most between init and gen, and decl and call
void lang_c_generate_action(bool isInit, bool defin, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i,j,k;
	bool isComplex;
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
		if(n->value && strlen(n->value)) fprintf(outFile, "// %s\n", n->value);

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
					lang_c_initialize(n->children[i], -1, outFile, nodes, errBuf);
				}else{
					// Set scope to this node
					scope = n;
					// Generate arguments
					lang_c_generate(true, n->children[i], -1, outFile, nodes, errBuf);
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
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}

		// Otherwise we print the full function body
		}else{
			// If it is not time to define this action, skip for now
			if(!defin) break;

			// Open the body block
			fprintf(outFile, "{\n");

			// Print declarations for any scope vars
			for(j=0; j<n->countScopeVars; ++j){
				if(n->scopeVars[j]->childType == PARAMETER) continue;
				if(n->scopeVars[j]->which == LITERAL_STRNG){
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					fprintf(outFile, " %s = adhoc_assignData(adhoc_createData(strcpy(malloc(%d), \"%s\")));\n"
						,n->scopeVars[j]->name
						,strlen(n->scopeVars[j]->value)+1
						,n->scopeVars[j]->value
					);
				}else if(n->scopeVars[j]->which == LITERAL_ARRAY){
					// Specially handle declaration of temporaries for array literals
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					fprintf(outFile, " %s = malloc(%d*sizeof("
						,n->scopeVars[j]->name
						,n->scopeVars[j]->countChildren
					);
					lang_c_printTypeName(n->scopeVars[j]->children[0]->children[0], outFile);
					fprintf(outFile, "));\n");
					for(k=0; k<n->scopeVars[j]->countChildren; ++k){
						lang_c_indent(indent+1, outFile);
						fprintf(outFile, "%s[%d] = "
							,n->scopeVars[j]->name
							,atoi(n->scopeVars[j]->children[k]->value)
						);
						lang_c_generate(false, n->scopeVars[j]->children[k]->children[0], -1, outFile, nodes, errBuf);
						fprintf(outFile, ";\n");
					}
				}else{
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					fprintf(outFile, " %s;\n", n->scopeVars[j]->name);
				}
			}

			// Print the child statements
			for(; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Generate children
				lang_c_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}

			// Reduce the ref counts on all complex vars in scope, before ending
			if(n->children[n->countChildren-1]->which != CONTROL_RETRN){
				for(i=0; i<n->countScopeVars; ++i){
					// Check if a complex type
					isComplex = false;
					switch(n->scopeVars[i]->dataType){
					case TYPE_STRNG:
					case TYPE_ARRAY:
					case TYPE_STRCT:
						isComplex = true;
					}
					if(!isComplex) continue;

					// Reduce the reference count
					lang_c_indent(indent+1, outFile);
					fprintf(outFile, "adhoc_unassignData(%s);\n", n->scopeVars[i]->name);
				}
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
				lang_c_initialize(n->children[i], -1, outFile, nodes, errBuf);
			}
			break;
		}

		// Indent this function call, and call it
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s(", n->name);

		// Special handling for library functions
		if(!strcmp(n->package, "System")){
			// ADHOC print function
			if(!strcmp(n->name, "adhoc_print")){
				fprintf(outFile, "\"");
				for(i=0; i<n->countChildren; ++i){
					switch(n->children[i]->dataType){
						case TYPE_BOOL: fprintf(outFile, "%%d"); break;
						case TYPE_INT: fprintf(outFile, "%%d"); break;
						case TYPE_FLOAT: fprintf(outFile, "%%f"); break;
						case TYPE_STRNG: fprintf(outFile, "%%s"); break;
						default:
							adhoc_errorNode = n->children[i];
							sprintf(
								errBuf
								,"Node %d: Datatype not acceptable for printing: %s"
								,n->children[i]->id
								,adhoc_dataType_names[n->children[i]->dataType]
							);
					}
				}
				fprintf(outFile, "\", ");
			}
		}

		// Print the action's arguments
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == ARGUMENT || n->children[i]->childType == PARAMETER){
				if(i>0) fprintf(outFile, ", ");
				lang_c_generate(false, n->children[i], -1, outFile, nodes, errBuf);
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
void lang_c_generate_group(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		// Initialize all children in order
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		// Generate all children in order
		for(i=0; i<n->countChildren; ++i){
			lang_c_generate(false, n->children[i], indent, outFile, nodes, errBuf);
		}
	}
}

// Controls vary greatly
void lang_c_generate_control(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	bool isComplex;
	switch(n->which){
	case CONTROL_IF:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Print the opening of the 'if' block
		lang_c_indent(indent, outFile);
		fprintf(outFile, "if(");

		// Print the condition
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				lang_c_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}

		// Close the 'if' line
		fprintf(outFile, "){\n");

		// Print the 'if' statements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType != IF) continue;
			lang_c_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Print the 'else' statements
		bool needElse = true;
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType != ELSE) continue;
			if(needElse){
				lang_c_indent(indent, outFile);
				fprintf(outFile, "}else{\n");
				needElse = false;
			}
			lang_c_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the whole 'if' block
		lang_c_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_LOOP:
		// Nothing to do during initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Declare scope vars
		for(i=0; i<n->countScopeVars; ++i){
			lang_c_indent(indent, outFile);
			lang_c_printTypeName(n->scopeVars[i], outFile);
			fprintf(outFile, " %s;\n", n->scopeVars[i]->name);
		}

		// Open for statement
		lang_c_indent(indent, outFile);
		fprintf(outFile, "for(");

		// Find the initialization and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION){
				// Generate initialization
				lang_c_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ");

		// Find the condition and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				// Generate condition
				lang_c_generate(false, n->children[i], -1, outFile, nodes, errBuf);
				break;
			}
		}
		fprintf(outFile, "; ){\n");

		// Print all the body elements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION) continue;
			if(n->children[i]->childType == CONDITION) continue;
			// Generate children
			lang_c_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
		}

		// Close the for statement
		lang_c_indent(indent, outFile);
		fprintf(outFile, "}\n");
		break;

	case CONTROL_SWITCH:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				// Initialize children
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		sprintf(errBuf, "SWITCH statements are not implemented yet :(");
		break;

	case CONTROL_CASE:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
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
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
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
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Just print the keyword and a semicolon
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s;\n", adhoc_nodeWhich_names[n->which]);
		break;

	case CONTROL_RETRN:
		if(isInit){
			// During initialization, add a return variable to the scope
			n->name = realloc(n->name, 13);
			snprintf(n->name, 13, "tmp%d", n->id);
			adhoc_assignScope(n, n->scope);

			// Initialize children
			for(i=0; i<n->countChildren; ++i){
				lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
			}
			break;
		}

		// Print the return var name
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s = ", n->name);

		// If returning a complex type, assign a reference to it
		isComplex = false;
		switch(n->dataType){
		case TYPE_STRNG:
		case TYPE_ARRAY:
		case TYPE_STRCT:
			isComplex = true;
		}
		if(isComplex) fprintf(outFile, "adhoc_assignData(");

		// Generate the children
		for(i=0; i<n->countChildren; ++i){
			lang_c_generate(false, n->children[i], 0, outFile, nodes, errBuf);
		}

		// Close reference assignment
		if(isComplex) fprintf(outFile, ")");
		fprintf(outFile, ";\n");

		// Reduce the ref counts on all complex vars in scope, before returning
		for(i=0; i<n->scope->countScopeVars; ++i){
			// Skip the return node itself;
			if(n->scope->scopeVars[i] == n) continue;

			// Check if a complex type
			isComplex = false;
			switch(n->scope->scopeVars[i]->dataType){
			case TYPE_STRNG:
			case TYPE_ARRAY:
			case TYPE_STRCT:
				isComplex = true;
			}
			if(!isComplex) continue;

			// Reduce the reference count
			lang_c_indent(indent, outFile);
			fprintf(outFile, "adhoc_unassignData(%s);\n", n->scope->scopeVars[i]->name);
		}

		// Print the actual return
		lang_c_indent(indent, outFile);
		fprintf(outFile, "return %s;\n", n->name);
		break;
	}
}

// Generation rules for operators
void lang_c_generate_operator(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		if(n->childType == STATEMENT) lang_c_indent(indent, outFile);
		switch(n->which){
			case OPERATOR_NOT:
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_ARIND:
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, "[");
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
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
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_TRNIF:
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " ? ");
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " : ");
				lang_c_generate(false, n->children[2], indent+1, outFile, nodes, errBuf);
				break;
		}
		if(n->childType == STATEMENT) fprintf(outFile, ";\n");
	}
}

// Generation rules for assignments
void lang_c_generate_assignment(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	bool isComplex;
	if(isInit){
		// Initialize the children and pass their types to the assignment and storage
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		if(n->childType == STATEMENT
				|| n->childType == IF
				|| n->childType == ELSE)
			lang_c_indent(indent, outFile);
		switch(n->which){
			case ASSIGNMENT_INCPR:
			case ASSIGNMENT_DECPR:
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				break;
			case ASSIGNMENT_INCPS:
			case ASSIGNMENT_DECPS:
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				break;
			case ASSIGNMENT_EQUAL:
				isComplex = false;
				switch(n->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_STRCT:
					isComplex = true;
				}
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				if(isComplex) fprintf(outFile, "adhoc_assignData(");
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				if(isComplex) fprintf(outFile, ")");
				break;
			case ASSIGNMENT_PLUS:
			case ASSIGNMENT_MINUS:
			case ASSIGNMENT_TIMES:
			case ASSIGNMENT_DIVBY:
			case ASSIGNMENT_MOD:
			case ASSIGNMENT_EXP:
			case ASSIGNMENT_OR:
			case ASSIGNMENT_AND:
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
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
void lang_c_generate_variable(bool isInit, bool defin, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
		if(n->childType == PARAMETER){
			lang_c_printTypeName(n, outFile);
		}
	}else{
		if(n->childType == PARAMETER){
			if(defin){
				lang_c_printTypeName(n, outFile);
				fprintf(outFile, " %s", n->name);
			}
		}else if(n->childType == INITIALIZATION){
			fprintf(outFile, "%s = ", n->name);
		}else{
			fprintf(outFile, "%s", n->name);
		}
		if(n->childType != PARAMETER || !defin){
			for(i=0; i<n->countChildren; ++i){
				lang_c_generate(false, n->children[i], indent+1, outFile, nodes, errBuf);
			}
		}
	}
}

// Generation rules for literals
void lang_c_generate_literal(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
		switch(n->which){
		case LITERAL_STRNG:
		case LITERAL_ARRAY:
		case LITERAL_HASH:
		case LITERAL_STRCT:
			n->name = realloc(n->name, 13);
			snprintf(n->name, 13, "tmp%d", n->id);
			adhoc_assignScope(n, n->scope);
			break;
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
				fprintf(outFile, "(char*)adhoc_getData(%s)", n->name);
				break;
			case LITERAL_ARRAY:
				fprintf(outFile, "%s", n->name);
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
void lang_c_initialize(ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	// Handle different node types
	switch(n->nodeType){
		case TYPE_NULL: lang_c_generate_null(true, n, indent, outFile, nodes, errBuf); break;
		case ACTION: lang_c_generate_action(true, false, n, indent, outFile, nodes, errBuf); break;
		case GROUP: lang_c_generate_group(true, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: lang_c_generate_control(true, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: lang_c_generate_operator(true, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: lang_c_generate_assignment(true, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: lang_c_generate_variable(true, false, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: lang_c_generate_literal(true, n, indent, outFile, nodes, errBuf); break;
	}
}
// Function to generate code from an AST node
void lang_c_generate(bool defin, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	switch(n->nodeType){
		case TYPE_NULL: lang_c_generate_null(false, n, indent, outFile, nodes, errBuf); break;
		case ACTION: lang_c_generate_action(false, defin, n, indent, outFile, nodes, errBuf); break;
		case GROUP: lang_c_generate_group(false, n, indent, outFile, nodes, errBuf); break;
		case CONTROL: lang_c_generate_control(false, n, indent, outFile, nodes, errBuf); break;
		case OPERATOR: lang_c_generate_operator(false, n, indent, outFile, nodes, errBuf); break;
		case ASSIGNMENT: lang_c_generate_assignment(false, n, indent, outFile, nodes, errBuf); break;
		case VARIABLE: lang_c_generate_variable(false, defin, n, indent, outFile, nodes, errBuf); break;
		case LITERAL: lang_c_generate_literal(false, n, indent, outFile, nodes, errBuf); break;
	}
}

// Hook function for generalized initialization
void lang_c_init(ASTnode* n, FILE* outFile, hashMap* nodes, bool exec, char* errBuf){
	countFuncs = 0;
	sizeFuncs = 2;
	functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
	if(exec){
		fprintf(outFile, "#include <stdlib.h>\n#include <string.h>\n#include <libadhoc.h>\n");
	}
	lang_c_initialize(n, 0, outFile, nodes, errBuf);
}
// Hook function for generalized code generation
void lang_c_gen(ASTnode* n, FILE* outFile, hashMap* nodes, bool exec, char* errBuf){
	int i;
	bool isComplex;
	for(i=0; i<countFuncs; ++i){
		lang_c_generate(true, functions[i], 0, outFile, nodes, errBuf);
	}
	if(exec && i){
		// Throw a warning if the main action has parameters
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == PARAMETER){
				adhoc_errorNode = n;
				sprintf(errBuf, "If generating an executable in C, main action must not have paramters.");
			}
		}

		// Determine whether the main action returns a complex type
		isComplex = false;
		switch(n->dataType){
		case TYPE_STRNG:
		case TYPE_ARRAY:
		case TYPE_STRCT:
			isComplex = true;
		}

		// To make an executable, we need som boilerplate
		fprintf(outFile, "\n// Main function for execution\n");
		fprintf(outFile, "int main(int argc, char **argv){\n");
		lang_c_indent(1, outFile);
		if(isComplex) fprintf(outFile, "adhoc_unassignData(");
		fprintf(outFile, "%s()", n->name);
		if(isComplex) fprintf(outFile, ")");
		fprintf(outFile, ";\n");
		lang_c_indent(1, outFile);
		fprintf(outFile, "return 0;\n");
		fprintf(outFile, "}\n");
	}
	free(functions);
}

#endif
