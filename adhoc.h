#ifndef ADHOC_H
#define ADHOC_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"
#include "c.h"
#include "javascript.h"

// An enumeration of config file sections
typedef enum {
	CONFIG_STATE_NULL
	,CONFIG_STATE_COMPILER
	,CONFIG_STATE_MODULES
} config_state;

// A simple structure to hold locations of things
typedef struct itemLocation {
	char* item;
	char* location;
} itemLocation;

// Free memory from an itemLocation {
void adhoc_destroyItemLocation(void* v){
	if(!v) return;
	itemLocation* i = (itemLocation*) v;
	free(i->item);
	free(i->location);
	free(i);
}

// Config options
char* ADHOC_VERSION_NUMBER = "1.0.0";
char ADHOC_CONFIG_LOCATION[100];
bool ADHOC_INFO_ONLY = false;
char ADHOC_TARGET_LANGUAGE[30];
bool ADHOC_OUPUT_COLOR = false;
bool ADHOC_DEBUG_INFO = false;
bool ADHOC_EXECUTABLE = false;
hashMap_uint ADHOC_ESTIMATED_NODE_COUNT = 100;

// A hashMap of language module locations
hashMap* moduleMap;
// A hashMap of all the AST nodes
hashMap* nodeMap;
// A placeholder node during AST building
ASTnode* readNode,* ASTroot;

// Check if a file is accessible in a particular mode
bool fileAcc(char* path, char* mode){
	FILE* f;
	return (f=fopen(path, mode)) && !fclose(f);
}

// A walkable simple print function
void adhoc_printNode(ASTnode* n, int d){
	char* buf = calloc(20, sizeof(char));
	sprintf(buf, "%%-%ds%%2d %%s%%s (%%s)\n", d*3);
	fprintf(
		stderr
		,buf
		,""
		,n->id
		,(n->parentId ? (n->countChildren ? "+-" : "--") : "##")
		,adhoc_getNodeLabel(n)
		,adhoc_getNodeSubLabel(n)
	);
	free(buf);
}

// A walkable name checker
void adhoc_renameNode(ASTnode* n, int d){
	char* p;
	while(p = strchr(n->package, ' ')) *p = '_';
	while(p = strchr(n->name, ' ')) *p = '_';
	if(!strcmp(n->package, "System")){
		char* buf = malloc(strlen(n->name)+1);
		strcpy(buf, n->name);
		char* libraryPrepend;
		if(!strcmp(ADHOC_TARGET_LANGUAGE, "c")){
			libraryPrepend = "adhoc_";
		}else if(!strcmp(ADHOC_TARGET_LANGUAGE, "javascript")){
			libraryPrepend = "Adhoc.";
		}
		int prependLen = strlen(libraryPrepend);
		n->name = realloc(n->name, strlen(n->name)+prependLen+1);
		strcpy(n->name, libraryPrepend);
		strcpy(n->name+prependLen, buf);
		free(buf);
	}
}

// Post-walkable function for determining the data-Type of a node
void adhoc_determineType(ASTnode* n, int d){
	if(n->reference){
		n->dataType = n->reference->dataType;
		return;
	}
	int i;
	switch(n->which){
	case WHICH_NULL:
	case GROUP_SERIAL:
	case CONTROL_IF:
	case CONTROL_LOOP:
	case CONTROL_SWITCH:
	case CONTROL_CASE:
	case CONTROL_FORK:
	case CONTROL_CNTNU:
	case CONTROL_BREAK:
		n->dataType = TYPE_VOID;
		break;

	case ACTION_DEFIN:
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->which != CONTROL_RETRN) continue;
			n->dataType = n->children[i]->dataType;
			break;
		}
		break;

	case ACTION_CALL:
		n->dataType = TYPE_VOID;
// TODO
		break;

	case CONTROL_RETRN:
		for(i=0; i<n->countChildren; ++i){
			if(n->children[i]->dataType != TYPE_VOID){
				n->dataType = n->children[i]->dataType;
			}
		}
		break;

	case ASSIGNMENT_NEGPR:
	case ASSIGNMENT_NEGPS:
	case ASSIGNMENT_OR:
	case ASSIGNMENT_AND:
		n->children[0]->dataType = TYPE_BOOL;
	case OPERATOR_OR:
	case OPERATOR_AND:
	case OPERATOR_NOT:
	case OPERATOR_EQUIV:
	case OPERATOR_GRTTN:
	case OPERATOR_LESTN:
	case OPERATOR_GRTEQ:
	case OPERATOR_LESEQ:
	case OPERATOR_NOTEQ:
	case LITERAL_BOOL:
		n->dataType = TYPE_BOOL;
		break;

	case OPERATOR_PLUS:
	case OPERATOR_MINUS:
	case OPERATOR_TIMES:
	case OPERATOR_DIVBY:
	case OPERATOR_MOD:
	case OPERATOR_EXP:
		n->dataType = adhoc_resolveTypes(
			n->children[0]->dataType
			,n->children[1]->dataType
		);
		break;

	case OPERATOR_ARIND:
		n->dataType = TYPE_VOID;
// TODO
		break;

	case OPERATOR_TRNIF:
		n->dataType = adhoc_resolveTypes(
			n->children[1]->dataType
			,n->children[2]->dataType
		);
		break;

	case ASSIGNMENT_INCPR:
	case ASSIGNMENT_INCPS:
	case ASSIGNMENT_DECPR:
	case ASSIGNMENT_DECPS:
		n->dataType = TYPE_VOID;
// TODO
		n->children[0]->dataType = n->dataType;
		break;

	case ASSIGNMENT_EQUAL:
		n->dataType = n->children[1]->dataType;
		n->children[0]->dataType = n->dataType;
		break;

	case ASSIGNMENT_PLUS:
	case ASSIGNMENT_MINUS:
	case ASSIGNMENT_TIMES:
	case ASSIGNMENT_DIVBY:
	case ASSIGNMENT_MOD:
	case ASSIGNMENT_EXP:
		n->dataType = n->children[1]->dataType;
// TODO
		n->children[0]->dataType = n->dataType;
		break;

	case VARIABLE_ASIGN:
		break;

	case VARIABLE_EVAL:
// TODO: Get type from reference
		break;

	case LITERAL_INT:
		n->dataType = TYPE_INT;
		break;
	case LITERAL_FLOAT:
		n->dataType = TYPE_FLOAT;
		break;
	case LITERAL_STRNG:
		n->dataType = TYPE_STRNG;
		break;
	case LITERAL_ARRAY:
		n->dataType = TYPE_ARRAY;
		break;
	case LITERAL_HASH:
		n->dataType = TYPE_HASH;
		break;
	case LITERAL_STRCT:
		n->dataType = TYPE_STRCT;
		break;
	}
}

// Simple function for hashing other structs
hashMap_uint adhoc_hashItemLocation(void* i){
	return hashMap_hashString(((itemLocation*) i)->item);
}

// Function to handle command line variables
void adhoc_handleCLIVariable(char* var, char* val, char* errBuf){
	if(val == NULL){
		char* p = strchr(var, '=');
		if(p){
			*p = '\0';
			adhoc_handleCLIVariable(var, p+1, errBuf);
			return;
		}
	}
	// Config file variable
	if(!strcmp(var, "config")){
		memset(ADHOC_CONFIG_LOCATION, 0, 100);
		strcpy(ADHOC_CONFIG_LOCATION, val);
		return;
	}
	// Debug info variable
	if(!strcmp(var, "debug")){
		ADHOC_DEBUG_INFO = true;
		return;
	}
	// Executable variable
	if(!strcmp(var, "executable")){
		ADHOC_EXECUTABLE = true;
		return;
	}
	// Help variable
	if(!strcmp(var, "help")){
		ADHOC_INFO_ONLY = true;
		printf("[1mNAME[22m\n\tADHOC - Action-Driven Human-Oriented Compiler\n\n");
		printf("[1mDESCRIPTION[22m\n\tParses a programming logic file and generates source code in a target\n\tlanguage. Expects a FILENAME (typically with extension '.adh') which\n\tcontains a representation of programming logic. If no FILENAME is\n\tprovided, ADHOC takes its input from stdin. So you can, for instance,\n\tsend logic content in via a pipe:\n\t\tcat foo.adh | adhoc -l c\n\t\t./logic_script.sh | adhoc -l sh -o other_script.sh\n\n");
		printf("[1mUSAGE SYNOPSIS[22m\n\tadhoc [ARGUMENT]... [FILENAME]\n\n");
		printf("[1mARGUMENTS[22m\n");
		printf("\t-c [1;4mfilename[22;24m, --config=[1;4mfilename[22;24m\n\t\tUse [1;4mfilename[22;24m as ADHOC's configuration file instead of the\n\t\tdefault adhoc.ini file.\n\n");
		printf("\t-d, --debug\n\t\tPrint out debug information while parsing the file.\n\n");
		printf("\t-e, --executable\n\t\tIn addition to generating the target language code from the\n\t\tinput logic, ADHOC will also include code necessary to execute\n\t\tthe output program (e.g. when generating C code, it will include\n\t\ta 'main()' function).\n\n");
		printf("\t-h, --help\n\t\tPrint this usage information.\n\n");
		printf("\t-l [1;4mlang[22;24m, --language=[1;4mlang[22;24m\n\t\tSet the target language for code generation to [1;4mlang[22;24m. This\n\t\toverrides the value set for ADHOC_TARGET_LANGUAGE in the config\n\t\tfile.\n\n");
		printf("\t-o [1;4mfilename[22;24m, --outfile=[1;4mfilename[22;24m\n\t\tDirects generated target code to [1;4mfilename[22;24m instead of stdout.\n\t\tSimilar to adhoc ... > [1;4mfilename[22;24m, but won't affect version info,\n\t\tetc.\n\n");
		printf("\t-v, --version\n\t\tPrint ADHOC version information.\n\n");
		printf("[1mLICENSE[22m\n");
		printf("\tOpen Source Under GPL v3 2014\n");
		printf("\tMore info at: https://github.com/pieman72/adhoc\n\n");
		return;
	}
	// Language variable
	if(!strcmp(var, "language")){
		memset(ADHOC_TARGET_LANGUAGE, 0, 30);
		strncpy(ADHOC_TARGET_LANGUAGE, val, 29);
		return;
	}
	// Outfile change variable
	if(!strcmp(var, "outfile")){
		if(!freopen(val, "w", stdout)){
			sprintf(errBuf, "Could not open file for writing: %-45s", val);
		}
		return;
	}
	// Version information variable
	if(!strcmp(var, "version")){
		ADHOC_INFO_ONLY = true;
		printf("[4;38;5;242mADHOC Version:[24;39m\n%s\n", ADHOC_VERSION_NUMBER);
		return;
	}
	sprintf(errBuf, "Unknown CLI variable: '--%-50s'\n", var);
}

// Function to handle command line flags
void adhoc_handleCLIFlag(char flag, char* val, char* errBuf){
	// Switch converts command line flags into full length variables
	switch(flag){
		case 'c': adhoc_handleCLIVariable("config", val, errBuf); return;
		case 'd': adhoc_handleCLIVariable("debug", val, errBuf); return;
		case 'e': adhoc_handleCLIVariable("executable", val, errBuf); return;
		case 'h': adhoc_handleCLIVariable("help", val, errBuf); return;
		case 'l': adhoc_handleCLIVariable("language", val, errBuf); return;
		case 'o': adhoc_handleCLIVariable("outfile", val, errBuf); return;
		case 'v': adhoc_handleCLIVariable("version", val, errBuf); return;
		default: sprintf(errBuf, "Unknown CLI flag: '-%c'", flag); return;
	}
}

// Function to handle configuration file variables
void adhoc_handleConfigVariable(char* var, char* val, char* errBuf){
	if(val == NULL){
		char* p = strchr(var, '=');
		if(p){
			*p = '\0';
			adhoc_handleConfigVariable(var, p+1, errBuf);
			return;
		}
	}
	if(!strcmp(var, "ADHOC_TARGET_LANGUAGE") && !strlen(ADHOC_TARGET_LANGUAGE)){
		strncpy(ADHOC_TARGET_LANGUAGE, val, 29);
		return;
	}
	if(!strcmp(var, "ADHOC_OUPUT_COLOR")){
		ADHOC_OUPUT_COLOR = !strcmp(val, "true");
		return;
	}
}

// Function to store the locations of various language modules
void adhoc_setModuleLocation(char* lang, char* loc, char* errBuf){
	if(loc == NULL){
		char* p = strchr(lang, '=');
		if(p){
			*p = '\0';
			adhoc_setModuleLocation(lang, p+1, errBuf);
			return;
		}
		sprintf(errBuf, "Module location improperly defined for %-30s", lang);
		return;
	}
	itemLocation* i = malloc(sizeof(itemLocation));
	i->item = malloc(strlen(lang)+1);
	i->location = malloc(strlen(loc)+1);
	strcpy(i->item, lang);
	strcpy(i->location, loc);
	hashMap_add(moduleMap, (void*) i);
}

// Initialize ADHOC. Read configuration files / arguments
void adhoc_init(int argc, char** argv, char* errBuf){
	// Check likely config file locations. Can be overridden by CLI arg
	memset(ADHOC_CONFIG_LOCATION, 0, 100);
	if(fileAcc("./adhoc.ini", "r")){
		strcpy(ADHOC_CONFIG_LOCATION, "./adhoc.ini");
	}else if(fileAcc("~/adhoc.ini", "r")){
		strcpy(ADHOC_CONFIG_LOCATION, "~/adhoc.ini");
	}else if(fileAcc("/usr/lib/adhoc/adhoc.ini", "r")){
		strcpy(ADHOC_CONFIG_LOCATION, "/usr/lib/adhoc/adhoc.ini");
	}
	memset(ADHOC_TARGET_LANGUAGE, 0, 30);

	// Handle command line arguments
	int i;
	for(i=1; i<argc; ++i){
		// Understood argument patterns
		if(argv[i][0] == '-'){
			if(argv[i][1] == '-'){
				// Arguments of the form:  --arg=val
				adhoc_handleCLIVariable(argv[i]+2, NULL, errBuf);
			}else if(i<argc-2 && argv[i+1][0]!='-'){
				// Arguments of the form:  -a [val]
				adhoc_handleCLIFlag(argv[i][1], argv[i+1], errBuf);
				++i;
			}else{
				// Arguments of the form:  -a
				adhoc_handleCLIFlag(argv[i][1], NULL, errBuf);
			}
			if(strlen(errBuf)) return;
		// The last argument should hold the file to be parsed. If so, reroute stdin
		}else if(i==argc-1){
			if(!freopen(argv[i], "r", stdin)){
				sprintf(errBuf, "Could not open file for parsing: %-40s", argv[i]);
				return;
			}
		// Argument given in the wrong form
		}else{
			sprintf(errBuf, "Unknown argument: %30s. Use 'adhoc -h' for help.", argv[i]);
			return;
		}
	}

	// If all that was wanted was help or version info, return now
	if(ADHOC_INFO_ONLY) return;

	// If a config file exists, Read in lines of the config file and process them
	if(strlen(ADHOC_CONFIG_LOCATION)){
		// Load the config file
		FILE* conf;
		if(!(conf = fopen(ADHOC_CONFIG_LOCATION, "r"))){
			sprintf(errBuf, "Could not open config file: %-50s", ADHOC_CONFIG_LOCATION);
		}

		// Prepare to read lines of input
		int len, size;
		size = 30;
		char* line;
		line = (char*) malloc(size);
		config_state confStat;
		confStat = CONFIG_STATE_NULL;
		do{
			// Read the line and skip it if it failes or is empty
			len = getline(&line, &size, conf);
			if(len<0) break;
			if(len<3 && !feof(conf)) continue;

			// Remove line-ending characters
			if(len>1 && (line[len-2]=='\r' || line[len-2]=='\n')) line[len-2] = '\0';
			if(len>0 && (line[len-1]=='\r' || line[len-1]=='\n')) line[len-1] = '\0';

			// Handle different kinds of config lines
			switch(line[0]){
				// Comments are ignored
				case '#': break;
				// Block delimiters change config state
				case '[':
					line[strlen(line)-1] = '\0';
					if(!strcmp(line+1, "compiler")){
						confStat = CONFIG_STATE_COMPILER;
						break;
					}
					if(!strcmp(line+1, "modules")){
						confStat = CONFIG_STATE_MODULES;
						moduleMap = hashMap_create(&adhoc_hashItemLocation, 10);
						break;
					}
				// All other lines should be options
				default:
					// In the compiler state, we are setting runtime options
					if(confStat == CONFIG_STATE_COMPILER){
						adhoc_handleConfigVariable(line, NULL, errBuf);
						if(strlen(errBuf)) return;
					// In the modules state, we are reading a list of module locations
					}else if(confStat == CONFIG_STATE_MODULES){
						adhoc_setModuleLocation(line, NULL, errBuf);
						if(strlen(errBuf)) return;
					}
					break;
			}
		}while(!feof(conf));
		fclose(conf);
		free(line);
	}

	// We're ready to parse. Set up the data structures
	nodeMap = hashMap_create(&adhoc_hashNode, ADHOC_ESTIMATED_NODE_COUNT);
	readNode = adhoc_createBlankNode();
	return;
}

// Insert a node into the abstract syntax tree
void adhoc_insertNode(ASTnode* n){
	// Add the new node to the node map
	hashMap_add(nodeMap, (void*)n);

	// Capture the root node
	if(!n->parentId){
		ASTroot = n;
		return;
	}

	// Fetch the node's parent by its id
	ASTnode* parent;
	parent = (ASTnode*) hashMap_retrieve(nodeMap, adhoc_hashParent((void*)n));
	n->parent = parent;

	// If the parent has no children, allocate the children array
	if(!parent->countChildren){
		parent->children = malloc(sizeof(ASTnode*));
		parent->sizeChildren = 1;
	// If the parent is full of children, double the children array
	}else if(parent->countChildren == parent->sizeChildren){
		parent->sizeChildren *= 2;
		parent->children = realloc(parent->children, parent->sizeChildren*sizeof(ASTnode*));
	}
	// Add the node to its parent
	parent->children[parent->countChildren++] = n;

	// Determine the node's scope
	if(parent->which==ACTION_DEFIN){
		adhoc_assignScope(n, parent);
	}else if(parent->scope){
		adhoc_assignScope(n, parent->scope);
	}
}

// Validate and optimize the abstract syntax tree
void adhoc_validate(char* errBuf){
	if(ADHOC_DEBUG_INFO){
		adhoc_treeWalk(adhoc_printNode, ASTroot, 0);
		fprintf(
			stderr
			,"\n\n%s-- Begin Generation --%s\n"
			,(ADHOC_OUPUT_COLOR ? "[38;5;63m" : "")
			,(ADHOC_OUPUT_COLOR ? "[39m" : "")
		);
	}

	// Determine scopes for all nodes

	// Rename system calls, and nodes with spaces in their names
	adhoc_treeWalk(adhoc_renameNode, ASTroot, 0);

	// Determine node dataTypes
	adhoc_treePostWalk(adhoc_determineType, ASTroot, 0);
}

// Generate the target language code
void adhoc_generate(char* errBuf){
	if(!strcmp(ADHOC_TARGET_LANGUAGE, "c")){
		lang_c_init(ASTroot, stdout, nodeMap, ADHOC_EXECUTABLE, errBuf);
		printf("\n");
		lang_c_gen(ASTroot, stdout, nodeMap, ADHOC_EXECUTABLE, errBuf);
	}else if(!strcmp(ADHOC_TARGET_LANGUAGE, "javascript")){
		lang_javascript_init(ASTroot, stdout, nodeMap, ADHOC_EXECUTABLE, errBuf);
		printf("\n");
		lang_javascript_gen(ASTroot, stdout, nodeMap, ADHOC_EXECUTABLE, errBuf);
	}else{
		sprintf(errBuf, "Target language \"%s\" not recognized", ADHOC_TARGET_LANGUAGE);
	}
}

// Clean up ADHOC
int adhoc_free(){
	hashMap_destroy(nodeMap, adhoc_destroyNode);
	hashMap_destroy(moduleMap, adhoc_destroyItemLocation);
	adhoc_destroyNode(readNode);
}

#endif
