#ifndef C_H
#define C_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wswitch"

// Keep track of whether we are generating an executable
bool execMode;

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
	if(n->dataType==TYPE_STRNG
			|| n->dataType==TYPE_ARRAY
			|| n->dataType==TYPE_HASH
		){
		fprintf(o, "adhoc_data*");
		return;
	}
	fprintf(o, "%s", adhoc_dataType_names[n->dataType]);
}

// C default values for data types
void lang_c_printTypeDefault(ASTnode* n, FILE* o){
	fprintf(o, "%s", adhoc_dataType_defaults[n->dataType]);
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

		// Print the action's parameters if it is not the root during exec mode
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == PARAMETER){
				if(n->parent || !execMode){
					if(i>0) fprintf(outFile, ", ");
					if(isInit){
						// Set scope to this node
						scope = n;
						// Initialize arguments
						lang_c_initialize(
							n->children[i]
							,0
							,outFile
							,nodes
							,errBuf
						);
					}else{
						// Set scope to this node
						scope = n;
						// Generate arguments
						lang_c_generate(
							true
							,n->children[i]
							,0
							,outFile
							,nodes
							,errBuf
						);
					}
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}

		// Otherwise we print the full function body
		}else{
			// If it is not time to define this action, skip for now
			if(!defin) break;

			// Open the body block
			fprintf(outFile, "{\n");

			// Increment references on complex parameters
			for(k=0; k<n->countChildren; ++k){
				if(n->children[k]->childType != PARAMETER) break;

				// Check if a complex type
				isComplex = false;
				switch(n->children[k]->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(!isComplex) continue;

				// Add reference incrementer
				lang_c_indent(indent+1, outFile);
				fprintf(outFile, "adhoc_referenceData(%s);\n"
					,n->children[k]->name
				);
			}
			if(k) fprintf(outFile, "\n");

			// Print declarations for any scope vars
			char* dt;
			bool declCommented = false;
			for(j=0; j<n->countScopeVars; ++j){
				// Skip parameters and unnamed vars
				if(n->scopeVars[j]->childType == PARAMETER) continue;
				if(!strlen(n->scopeVars[j]->name)) continue;

				// Comment the declarations
				if(!declCommented){
					lang_c_indent(indent+1, outFile);
					fprintf(outFile, "// Declare variables in scope\n");
					declCommented = true;
				}

				// Handle different datatypes differently
				switch(n->scopeVars[j]->dataType){
				case TYPE_STRNG:
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					if(n->scopeVars[j]->nodeType == LITERAL){
						fprintf(outFile, " %s = adhoc_createString(\"%s\");\n"
							,n->scopeVars[j]->name
							,n->scopeVars[j]->value
						);
					}else{
						fprintf(outFile, " %s = NULL;\n"
							,n->scopeVars[j]->name
						);
					}
					break;

				case TYPE_ARRAY:
					// Handle declaration of temporaries for array literals
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					if(n->scopeVars[j]->nodeType == LITERAL){
						switch(n->scopeVars[j]->childDataType){
						case TYPE_BOOL: dt = "DATA_BOOL"; break;
						case TYPE_INT: dt = "DATA_INT"; break;
						case TYPE_FLOAT: dt = "DATA_FLOAT"; break;
						case TYPE_STRNG: dt = "DATA_STRING"; break;
						case TYPE_ARRAY: dt = "DATA_ARRAY"; break;
						case TYPE_HASH: dt = "DATA_HASH"; break;
						case TYPE_STRCT: dt = "DATA_STRUCT"; break;
						default: dt = "DATA_VOID"; break;
						}
						fprintf(outFile, " %s = adhoc_referenceData(adhoc_createArray(%s, %d));\n"
							,n->scopeVars[j]->name
							,dt
							,n->scopeVars[j]->countChildren
						);
					}else{
						fprintf(outFile, " %s = NULL;\n"
							,n->scopeVars[j]->name
						);
					}
					break;

				default:
					lang_c_indent(indent+1, outFile);
					lang_c_printTypeName(n->scopeVars[j], outFile);
					fprintf(outFile, " %s = ", n->scopeVars[j]->name);
					lang_c_printTypeDefault(n->scopeVars[j], outFile);
					fprintf(outFile, ";\n");
				}
			}
			if(declCommented) fprintf(outFile, "\n");

			// Print the child statements
			if(n->countChildren){
				lang_c_indent(indent+1, outFile);
				fprintf(outFile, "// Body of %s\n", n->name);
			}
			for(; i<n->countChildren; ++i){
				// Set scope to this node
				scope = n;
				// Generate children
				lang_c_generate(
					false
					,n->children[i]
					,indent+1
					,outFile
					,nodes
					,errBuf
				);
			}

			// Reduce the ref counts on all complex vars in scope before ending
			if(n->children[n->countChildren-1]->which != CONTROL_RETRN){
				bool derefCommented = false;
				for(i=0; i<n->countScopeVars; ++i){
					// Check that the variable is named
					if(!strlen(n->scopeVars[i]->name)) continue;

					// Check if a complex type
					isComplex = false;
					switch(n->scopeVars[i]->dataType){
					case TYPE_STRNG:
					case TYPE_ARRAY:
					case TYPE_HASH:
					case TYPE_STRCT:
						isComplex = true;
					}
					if(!isComplex) continue;

					// Reduce the reference count
					if(!derefCommented){
						fprintf(outFile, "\n");
						lang_c_indent(indent+1, outFile);
						fprintf(
							outFile
							,"// Reduce references on complex scope vars\n"
						);
						derefCommented = true;
					}
					lang_c_indent(indent+1, outFile);
					fprintf(
						outFile
						,"adhoc_unreferenceData(%s);\n"
						,n->scopeVars[i]->name
					);
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
				lang_c_initialize(n->children[i], 0, outFile, nodes, errBuf);
			}
			break;
		}

		// Indent this function call
		if(n->childType == STATEMENT) lang_c_indent(indent, outFile);

		// Special handling for library functions
		if(!strcmp(n->package, "System")){
			// TODO: There's probably a better way than enumerating by name. Hash the names?
			// type - Returns the type (as an integer 0-9) of one complex argument
			if(!strcmp(n->name, "adhoc_type")){
				bool isComplex = false;
				switch(n->children[0]->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(isComplex) fprintf(outFile, "adhoc_type(");
				else{
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: Only complex dataTypes can be passed to type()"
						,n->children[0]->id
					);
				}

			// size - Returns the size (in bytes) of one argument
			}else if(!strcmp(n->name, "adhoc_size")){
				bool isComplex = false;
				switch(n->children[0]->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(isComplex) fprintf(outFile, "adhoc_sizeC(");
				else fprintf(outFile, "adhoc_sizeS(");

			// count - Returns the count of items in one argument
			}else if(!strcmp(n->name, "adhoc_count")){
				bool isComplex = false;
				switch(n->children[0]->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(isComplex) fprintf(outFile, "adhoc_countC(");
				else fprintf(outFile, "adhoc_countS(");

			// toString - prompt for a value
			}else if(!strcmp(n->name, "adhoc_toString")){
				switch(n->children[0]->dataType){
				case TYPE_VOID:
					fprintf(outFile, "adhoc_toStringS(DATA_VOID, ");
					break;
				case TYPE_BOOL:
					fprintf(outFile, "adhoc_toStringS(DATA_BOOL, ");
					break;
				case TYPE_INT:
					fprintf(outFile, "adhoc_toStringS(DATA_INT, ");
					break;
				case TYPE_FLOAT:
					fprintf(outFile, "adhoc_toStringS(DATA_FLOAT, ");
					break;
				default:
					fprintf(outFile, "adhoc_toStringC(");
				}

			// print - Prints arbitrarily many arguments
			}else if(!strncmp(n->name, "adhoc_print", 11)){
				bool isPrintLn = !strcmp(n->name+11, "ln");
				fprintf(outFile, "adhoc_print(\"");
				for(j=0; j<n->countChildren; ++j){
					switch(n->children[j]->dataType){
					case TYPE_BOOL: fprintf(outFile, "%%b"); break;
					case TYPE_INT: fprintf(outFile, "%%d"); break;
					case TYPE_FLOAT: fprintf(outFile, "%%f"); break;
					case TYPE_STRNG: fprintf(outFile, "%%s"); break;
					case TYPE_ARRAY:
					case TYPE_HASH:
					case TYPE_STRCT:
						fprintf(outFile, "%%_"); break;
					default:
						adhoc_errorNode = n->children[j];
						sprintf(
							errBuf
							,"Node %d: Datatype not printable: %s"
							,n->children[j]->id
							,adhoc_dataType_names[n->children[j]->dataType]
						);
					}
					if(isPrintLn) fprintf(outFile, "\\n");
				}
				fprintf(outFile, "\", ");

			// prompt - prompt for a value
			}else if(!strcmp(n->name, "adhoc_prompt")){
				if(n->children[0]->nodeType == VARIABLE){
					fprintf(outFile, "adhoc_prompt(");
					switch(n->children[0]->dataType){
					case TYPE_BOOL:
						fprintf(outFile, "DATA_BOOL, ");
						break;
					case TYPE_INT:
						fprintf(outFile, "DATA_INT, ");
						break;
					case TYPE_FLOAT:
						fprintf(outFile, "DATA_FLOAT, ");
						break;
					case TYPE_STRNG:
						fprintf(outFile, "DATA_STRING, ");
						break;
					default:
						fprintf(outFile, "DATA_VOID, ");
						adhoc_errorNode = n->children[0];
						sprintf(
							errBuf
							,"Node %d: Prompting for dataType %s is not implemented"
							,n->children[0]->id
							,adhoc_nodeType_names[n->children[0]->dataType]
						);
					}
				}else{
					fprintf(outFile, "DATA_VOID, ");
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: Can only prompt for variables. Found %s"
						,n->children[0]->id
						,adhoc_nodeType_names[n->children[0]->nodeType]
					);
				}

			// append to string - string extends first arg with second
			}else if(!strcmp(n->name, "adhoc_append_to_string")){
				if(n->children[0]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: First parameter to 'append to string' must be a string"
						,n->children[1]->id
					);
				}
				fprintf(outFile, "adhoc_append_to_string(\"");
				switch(n->children[1]->dataType){
				case TYPE_BOOL: fprintf(outFile, "%%b"); break;
				case TYPE_INT: fprintf(outFile, "%%d"); break;
				case TYPE_FLOAT: fprintf(outFile, "%%f"); break;
				case TYPE_STRNG: fprintf(outFile, "%%s"); break;
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					fprintf(outFile, "%%_"); break;
				default:
					adhoc_errorNode = n->children[1];
					sprintf(
						errBuf
						,"Node %d: Datatype cannot be appended to string: %s"
						,n->children[1]->id
						,adhoc_dataType_names[n->children[1]->dataType]
					);
				}
				fprintf(outFile, "\", ");

			// concat - converts each argument to a string then joins them all
			}else if(!strcmp(n->name, "adhoc_concat")){
				fprintf(outFile, "adhoc_concat(\"");
				for(j=0; j<n->countChildren; ++j){
					switch(n->children[j]->dataType){
					case TYPE_BOOL: fprintf(outFile, "%%b"); break;
					case TYPE_INT: fprintf(outFile, "%%d"); break;
					case TYPE_FLOAT: fprintf(outFile, "%%f"); break;
					case TYPE_STRNG: fprintf(outFile, "%%s"); break;
					case TYPE_ARRAY:
					case TYPE_HASH:
					case TYPE_STRCT:
						fprintf(outFile, "%%_"); break;
					default:
						adhoc_errorNode = n->children[j];
						sprintf(
							errBuf
							,"Node %d: Datatype cannot be concatenated: %s"
							,n->children[j]->id
							,adhoc_dataType_names[n->children[j]->dataType]
						);
					}
				}
				fprintf(outFile, "\", ");

			// substring - returns a copy from a string from an index of length
			}else if(!strcmp(n->name, "adhoc_substring")){
				if(n->children[0]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: First parameter to 'substring' must be a string"
						,n->children[1]->id
					);
				}
				if(n->children[1]->dataType != TYPE_INT){
					adhoc_errorNode = n->children[1];
					sprintf(
						errBuf
						,"Node %d: Second parameter to 'substring' must be an integer"
						,n->children[1]->id
					);
				}
				if(n->children[2]->dataType != TYPE_INT){
					adhoc_errorNode = n->children[2];
					sprintf(
						errBuf
						,"Node %d: Third parameter to 'substring' must be an integer"
						,n->children[2]->id
					);
				}
				fprintf(outFile, "%s(", n->name);

			// splice string - patches first with second from index of length
			}else if(!strcmp(n->name, "adhoc_splice_string")){
				if(n->children[0]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: First parameter to 'splice string' must be a string"
						,n->children[1]->id
					);
				}
				if(n->children[1]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[1];
					sprintf(
						errBuf
						,"Node %d: Second parameter to 'splice string' must be an string"
						,n->children[1]->id
					);
				}
				if(n->children[2]->dataType != TYPE_INT){
					adhoc_errorNode = n->children[2];
					sprintf(
						errBuf
						,"Node %d: Third parameter to 'splice string' must be an integer"
						,n->children[2]->id
					);
				}
				if(n->children[3]->dataType != TYPE_INT){
					adhoc_errorNode = n->children[3];
					sprintf(
						errBuf
						,"Node %d: Fourth parameter to 'splice string' must be an integer"
						,n->children[3]->id
					);
				}
				fprintf(outFile, "%s(", n->name);

			// find in string - gets first instance in string of substring
			}else if(!strcmp(n->name, "adhoc_find_in_string")){
				if(n->children[0]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: First parameter to 'find in string' must be a string"
						,n->children[1]->id
					);
				}
				if(n->children[1]->dataType != TYPE_STRNG){
					adhoc_errorNode = n->children[1];
					sprintf(
						errBuf
						,"Node %d: Second parameter to 'find in string' must be an string"
						,n->children[1]->id
					);
				}
				fprintf(outFile, "%s(", n->name);

			// append to array - pushes second arg onto the end of the first
			}else if(!strcmp(n->name, "adhoc_append_to_array")){
				if(n->children[0]->dataType != TYPE_ARRAY){
					adhoc_errorNode = n->children[0];
					sprintf(
						errBuf
						,"Node %d: First parameter to 'append to array' must be an array"
						,n->children[0]->id
					);
				}
				if(n->children[1]->dataType != n->children[0]->childDataType
						&& n->children[0]->childDataType != TYPE_MIXED
					){
					adhoc_errorNode = n;
					sprintf(
						errBuf
						,"Node %d: Mismatch between array dataType and type of item"
						,n->id
					);
				}
				fprintf(outFile, "adhoc_append_to_array(\"");
				switch(n->children[1]->dataType){
				case TYPE_BOOL: fprintf(outFile, "%%b"); break;
				case TYPE_INT: fprintf(outFile, "%%d"); break;
				case TYPE_FLOAT: fprintf(outFile, "%%f"); break;
				case TYPE_STRNG: fprintf(outFile, "%%s"); break;
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					fprintf(outFile, "%%_"); break;
				default:
					adhoc_errorNode = n->children[1];
					sprintf(
						errBuf
						,"Node %d: Datatype cannot be appended to an array: %s"
						,n->children[1]->id
						,adhoc_dataType_names[n->children[1]->dataType]
					);
				}
				fprintf(outFile, "\", ");

			// Unrecognized library function!
			}else{
				adhoc_errorNode = n;
				sprintf(
					errBuf
					,"Node %d: Datatype cannot be concatenated: %s"
					,n->id
					,adhoc_dataType_names[n->dataType]
				);
			}

		// If not a library function, then make a regular call
		}else{
			fprintf(outFile, "%s(", n->name);
		}

		// Print the action's arguments
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == ARGUMENT
					|| n->children[i]->childType == PARAMETER
				){
				if(n->countChildren>=4 && indent){
					fprintf(outFile, "\n");
					lang_c_indent(indent+1, outFile);
					if(i) fprintf(outFile, ",");
				}else if(i) fprintf(outFile, ", ");
				lang_c_generate(
					false
					,n->children[i]
					,0
					,outFile
					,nodes
					,errBuf
				);
			}
		}

		// Close the function call
		if(n->countChildren>=4 && indent){
			fprintf(outFile, "\n");
			lang_c_indent(indent, outFile);
		}
		fprintf(outFile, ")");

		// If this is the end of a statement, add a semicolon
		if(n->childType == STATEMENT
				|| n->childType == IF
				|| n->childType == ELSE
			){
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
			lang_c_generate(
				false
				,n->children[i]
				,indent
				,outFile
				,nodes
				,errBuf
			);
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}
			break;
		}

		// Print the opening of the 'if' block
		lang_c_indent(indent, outFile);
		fprintf(outFile, "if(");

		// Print the condition
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				lang_c_generate(
					false
					,n->children[i]
					,0
					,outFile
					,nodes
					,errBuf
				);
				break;
			}
		}

		// Close the 'if' line
		fprintf(outFile, "){\n");

		// Print the 'if' statements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType != IF) continue;
			lang_c_generate(
				false
				,n->children[i]
				,indent+1
				,outFile
				,nodes
				,errBuf
			);
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
			lang_c_generate(
				false
				,n->children[i]
				,indent+1
				,outFile
				,nodes
				,errBuf
			);
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
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
				lang_c_generate(
					false
					,n->children[i]
					,0
					,outFile
					,nodes
					,errBuf
				);
				break;
			}
		}
		fprintf(outFile, "; ");

		// Find the condition and print it
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == CONDITION){
				// Generate condition
				lang_c_generate(
					false
					,n->children[i]
					,0
					,outFile
					,nodes
					,errBuf
				);
				break;
			}
		}
		fprintf(outFile, "; ){\n");

		// Print all the body elements
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == INITIALIZATION) continue;
			if(n->children[i]->childType == CONDITION) continue;
			// Generate children
			lang_c_generate(
				false
				,n->children[i]
				,indent+1
				,outFile
				,nodes
				,errBuf
			);
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}
			break;
		}

		sprintf(errBuf, "SWITCH statements are not implemented yet :(");
		break;

	case CONTROL_CASE:
		// Nothing to do on initialization
		if(isInit){
			for(i=0; i<n->countChildren; ++i){
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
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
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}
			break;
		}

		// Just print the keyword and a semicolon
		lang_c_indent(indent, outFile);
		fprintf(outFile, "%s;\n", adhoc_nodeWhich_names[n->which]);
		break;

	case CONTROL_RETRN:;
		bool retVar = n->countChildren && (
			n->children[0]->nodeType == ACTION
			|| n->children[0]->nodeType == OPERATOR
			|| n->children[0]->nodeType == ASSIGNMENT
			|| n->children[0]->nodeType == VARIABLE
		);
		bool retVarComplex = false;
		if(retVar){
			switch(n->children[0]->dataType){
			case TYPE_STRNG:
			case TYPE_ARRAY:
			case TYPE_HASH:
			case TYPE_STRCT:
				retVarComplex = true;
			}
		}

		if(isInit){
			// During initialization, add a return variable to scope if needed
			if(retVar){
				n->name = realloc(n->name, 13);
				snprintf(n->name, 13, "tmp%d", n->id);
				adhoc_assignScope(n, n->scope);
			}

			// Initialize children
			for(i=0; i<n->countChildren; ++i){
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}
			break;
		}

		// Print the return var, if present
		if(retVar){
			// Print the return var name
			lang_c_indent(indent, outFile);
			fprintf(outFile, "%s = ", n->name);

			// Generate the children
			for(i=0; i<n->countChildren; ++i){
				if(retVarComplex) fprintf(outFile, "adhoc_referenceData(");
				lang_c_generate(
					false
					,n->children[i]
					,0
					,outFile
					,nodes
					,errBuf
				);
				if(retVarComplex) fprintf(outFile, ")");
			}

			// Close return var assignment
			fprintf(outFile, ";\n");
		}

		// Reduce the ref counts on all complex vars in scope, before returning
		bool derefCommented = false;
		for(i=0; i<n->scope->countScopeVars; ++i){
			// Skip the return node itself and any unnamed ones
			if(retVar && n->scope->scopeVars[i]==n) continue;
			if(!retVar
					&& n->countChildren
					&& n->scope->scopeVars[i]==n->children[0]
				) continue;
			if(!strlen(n->scope->scopeVars[i]->name)) continue;

			// Check if a complex type
			isComplex = false;
			switch(n->scope->scopeVars[i]->dataType){
			case TYPE_STRNG:
			case TYPE_ARRAY:
			case TYPE_HASH:
			case TYPE_STRCT:
				isComplex = true;
			}
			if(!isComplex) continue;

			// Leave a note
			if(!derefCommented){
				fprintf(outFile, "\n");
				lang_c_indent(indent, outFile);
				fprintf(
					outFile
					,"// Reduce references on complex scope variables and return\n"
				);
				derefCommented = true;
			}

			// Reduce the reference count
			lang_c_indent(indent, outFile);
			fprintf(
				outFile
				,"adhoc_unreferenceData(%s);\n"
				,n->scope->scopeVars[i]->name
			);
		}

		// Print the actual return
		if(retVarComplex){
			lang_c_indent(indent, outFile);
			fprintf(outFile, "--%s->refs;\n", n->name);
		}
		lang_c_indent(indent, outFile);
		fprintf(outFile, "return");
		if(retVar) fprintf(outFile, " %s", n->name);
		else if(n->countChildren){
			if(retVarComplex) fprintf(outFile, " %s", n->children[0]->name);
			else lang_c_generate(
				false
				,n->children[0]
				,0
				,outFile
				,nodes
				,errBuf
			);
		}
		fprintf(outFile, ";\n");
		break;
	}
}

// Generation rules for operators
void lang_c_generate_operator(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	bool isComplex, parens;
	if(isInit){
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		parens = needsParens(n);
		if(n->childType == STATEMENT) lang_c_indent(indent, outFile);
		if(parens) fprintf(outFile, "(");
		switch(n->which){
			case OPERATOR_NOT:
				fprintf(outFile, "%s", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_ARIND:
				isComplex = false;
				switch(n->children[0]->childDataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(n->childType==STORAGE && n->parent->which==ASSIGNMENT_EQUAL){
					fprintf(outFile, "adhoc_assignArrayData(%s, "
						,n->children[0]->name
					);
					lang_c_generate(
						false
						,n->children[1]
						,0
						,outFile
						,nodes
						,errBuf
					);
					fprintf(outFile, ", ");
					if(isComplex){
						lang_c_generate(
							false
							,n->parent->children[1]
							,0
							,outFile
							,nodes
							,errBuf
						);
						fprintf(outFile, ", 0");
					}else{
						fprintf(outFile, "NULL, ");
						lang_c_generate(
							false
							,n->parent->children[1]
							,0
							,outFile
							,nodes
							,errBuf
						);
					}
					fprintf(outFile, ")");
				}else{
					if(!isComplex){
						fprintf(outFile, "*(");
						lang_c_printTypeName(n, outFile);
						fprintf(outFile, "*)");
					}
					fprintf(outFile, "adhoc_get%sArrayData("
						,(isComplex ? "C" : "S")
					);
					lang_c_generate(
						false
						,n->children[0]
						,0
						,outFile
						,nodes
						,errBuf
					);
					fprintf(outFile, ", ");
					lang_c_generate(
						false
						,n->children[1]
						,0
						,outFile
						,nodes
						,errBuf
					);
					fprintf(outFile, ")");
				}
				break;
			case OPERATOR_PLUS:
			case OPERATOR_MINUS:
			case OPERATOR_TIMES:
			case OPERATOR_DIVBY:
			case OPERATOR_MOD:
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				lang_c_generate(false, n->children[1], indent+1, outFile, nodes, errBuf);
				break;
			case OPERATOR_EXP: // TODO
				adhoc_errorNode = n;
				sprintf(
					errBuf
					,"%s"
					,"Exponentiation is not currently implemented for C"
				);
				break;
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
		if(parens) fprintf(outFile, ")");
		if(n->childType == STATEMENT) fprintf(outFile, ";\n");
	}
}

// Generation rules for assignments
void lang_c_generate_assignment(bool isInit, ASTnode* n, short indent, FILE* outFile, hashMap* nodes, char* errBuf){
	int i;
	bool isComplex, parens;
	if(isInit){
		// Initialize the children and pass their types to the assignment and storage
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
	}else{
		parens = needsParens(n);
		if(n->childType == STATEMENT
				|| n->childType == IF
				|| n->childType == ELSE)
			lang_c_indent(indent, outFile);
		if(parens) fprintf(outFile, "(");
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
				lang_c_generate(false, n->children[0], indent+1, outFile, nodes, errBuf);
				if(n->children[0]->which == OPERATOR_ARIND) break;
				fprintf(outFile, " %s ", adhoc_nodeWhich_names[n->which]);
				isComplex = false;
				switch(n->children[1]->dataType){
				case TYPE_STRNG:
				case TYPE_ARRAY:
				case TYPE_HASH:
				case TYPE_STRCT:
					isComplex = true;
				}
				if(isComplex) fprintf(outFile, "adhoc_referenceData(");
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
		if(parens) fprintf(outFile, ")");
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
		if(defin){
			lang_c_indent(indent, outFile);
			lang_c_printTypeName(n, outFile);
			fprintf(outFile, " %s", n->name);
			if(n->countChildren){
				fprintf(outFile, " = ");
				lang_c_generate(
					false
					,n->children[0]
					,indent+1
					,outFile
					,nodes
					,errBuf
				);
			}
			fprintf(outFile, ";\n");
		}else{
			for(i=0; i<n->countChildren; ++i){
				lang_c_initialize(
					n->children[i]
					,indent
					,outFile
					,nodes
					,errBuf
				);
			}
			if(n->childType == PARAMETER){
				lang_c_printTypeName(n, outFile);
			}
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
			if(n->dataType == TYPE_VOID){
				adhoc_errorNode = n;
				sprintf(
					errBuf
					,"%s"
					,"Variable used but type could not be determined"
				);
			}
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
		// Initialize the children
		for(i=0; i<n->countChildren; ++i){
			lang_c_initialize(n->children[i], indent, outFile, nodes, errBuf);
		}
		// Containers need temp variables for initialization
		switch(n->which){
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
				fprintf(outFile, "adhoc_createString(\"%s\")", n->value);
				break;
			case LITERAL_ARRAY:
			case LITERAL_HASH:
			case LITERAL_STRCT:
				fprintf(outFile, "%s", n->name);
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
	int i,j;
	bool isComplex;
	for(i=0; i<n->countCmplxVals; ++i){
		for(j=0; j<n->cmplxVals[i]->countChildren; ++j){
			isComplex = false;
			switch(n->cmplxVals[i]->children[j]->children[0]->dataType){
			case TYPE_STRNG:
			case TYPE_ARRAY:
			case TYPE_HASH:
			case TYPE_STRCT:
				isComplex = true;
			}
			lang_c_indent(indent, outFile);
			fprintf(outFile, "adhoc_assignArrayData(%s, %d, "
				,n->cmplxVals[i]->name
				,atoi(n->cmplxVals[i]->children[j]->value)
			);
			if(!isComplex) fprintf(outFile, "NULL, ");
			lang_c_generate(
				false
				,n->cmplxVals[i]->children[j]->children[0]
				,0
				,outFile
				,nodes
				,errBuf
			);
			if(isComplex) fprintf(outFile, ", 0");
			fprintf(outFile, ");\n");
		}
	}
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
	execMode = exec;
	countFuncs = 0;
	sizeFuncs = 2;
	functions = realloc(functions, sizeFuncs * sizeof(ASTnode*));
	if(exec){
		fprintf(outFile, "#include <stdlib.h>\n#include <stdbool.h>\n#include <string.h>\n#include <libadhoc.h>\n");
	}
	lang_c_initialize(n, 0, outFile, nodes, errBuf);
}
// Hook function for generalized code generation
void lang_c_gen(ASTnode* n, FILE* outFile, hashMap* nodes, bool exec, char* errBuf){
	int i;
	bool isComplex;
	if(exec){
		// Print definitions for global vars
		if(n->countChildren && n->children[0]->childType == PARAMETER){
			fprintf(outFile, "\n// Global variables\n");
		}
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->childType == PARAMETER){
				lang_c_generate_variable(true, true, n->children[i], 0, outFile, nodes, errBuf);
			}
		}
	}
	for(i=0; i<countFuncs; ++i){
		lang_c_generate(true, functions[i], 0, outFile, nodes, errBuf);
	}
	if(exec && i){
		// Determine whether the main action returns a complex type
		isComplex = false;
		switch(n->dataType){
		case TYPE_STRNG:
		case TYPE_ARRAY:
		case TYPE_HASH:
		case TYPE_STRCT:
			isComplex = true;
		}

		// To make an executable, we need som boilerplate
		fprintf(outFile, "\n// Main function for execution\n");
		fprintf(outFile, "int main(int argc, char **argv){\n");
		lang_c_indent(1, outFile);
		if(isComplex) fprintf(outFile, "adhoc_unreferenceData(");
		fprintf(outFile, "%s()", n->name);
		if(isComplex) fprintf(outFile, ")");
		fprintf(outFile, ";\n");
		lang_c_indent(1, outFile);
		fprintf(outFile, "return 0;\n");
		fprintf(outFile, "}\n");
	}
	free(functions);
}

#pragma clang diagnostic pop
#endif
