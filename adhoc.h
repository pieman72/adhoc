#ifndef ADHOC_H
#define ADHOC_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

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

// Free memory from an itemLocaiton {
void adhoc_destroyItemLocation(void* v){
	if(!v) return;
	itemLocation* i = (itemLocation*) v;
	free(i->item);
	free(i->location);
	free(i);
}

// An abstract syntax tree node for use during parsing
typedef struct ASTnode {
	int id;
	int parentId;
	int nodeType;
	int which;
	int childType;
	char* package;
	char* name;
	char* value;
	unsigned short countChildren;
	unsigned short sizeChildren;
	struct ASTnode** children;
} ASTnode;

// Generic function pointer type for walks of the abstract syntax tree
typedef void (*walk_func)(ASTnode*, int);

// Config options
char* ADHOC_VERSION_NUMBER = "1.0.0";
char ADHOC_CONFIG_LOCATION[100];
bool ADHOC_INFO_ONLY = false;
char ADHOC_TARGET_LANGUAGE[30];
bool ADHOC_OUPUT_COLOR = false;
hashMap_uint ADHOC_ESTIMATED_NODE_COUNT = 100;

// A hashMap of language module locations
hashMap* moduleMap;
// A hashMap of all the AST nodes
hashMap* nodeMap;
// A placeholder node during AST building
ASTnode* readNode,* ASTroot;

// Allocate memory for a blank node
ASTnode* adhoc_createBlankNode(){
	ASTnode* ret = (ASTnode*) malloc(sizeof(ASTnode));
	ret->package = NULL;
	ret->name = NULL;
	ret->value = NULL;
	ret->countChildren = 0;
	ret->sizeChildren = 0;
	ret->children = NULL;
	return ret;
}

// Free memory from a node
void adhoc_destroyNode(void* v){
	if(!v) return;
	ASTnode* n = (ASTnode*) v;
	free(n->package);
	free(n->name);
	free(n->value);
	free(n->children);
	free(n);
}

// Determines the lable to use for rendering a node
const char* adhoc_getNodeLabel(ASTnode* n){
	if(n->name && strlen(n->name)) return n->name;
	return adhoc_nodeType_names[n->nodeType];
}

// Determines the sub-label to use for rendering a node
const char* adhoc_getNodeSubLabel(ASTnode* n){
	if(!n->parentId) return n->package;
	if(n->value && strlen(n->value)) return n->value;
	if(n->package && strlen(n->package)) return n->package;
	return adhoc_nodeWhich_names[n->which];
}

// A walkable simple print function
void adhoc_printNode(ASTnode* n, int d){
	char* buf = calloc(20, sizeof(char));
	sprintf(buf, "%%-%ds%%s%%s (%%s)\n", d*3);
	printf(
		buf
		,""
		,(n->parentId ? (n->countChildren ? "+- " : "-- ") : "## ")
		,adhoc_getNodeLabel(n)
		,adhoc_getNodeSubLabel(n)
	);
	free(buf);
}

// Walk the tree with a function to perform on each node
void adhoc_treeWalk(walk_func f, ASTnode* n, int d){
	f(n, d);
	int i;
	for(i=0; i<n->countChildren; ++i){
		adhoc_treeWalk(f, n->children[i], d+1);
	}
}

// Simple functions for hashing AST nodes and other structs
hashMap_uint adhoc_hashItemLocation(void* i){
	return hashMap_hashString(((itemLocation*) i)->item);
}
hashMap_uint adhoc_hashNode(void* n){
	return (hashMap_uint) ((ASTnode*) n)->id;
}
hashMap_uint adhoc_hashParent(void* n){
	return (hashMap_uint) ((ASTnode*) n)->parentId;
}

// Functiont to handle command line variables
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
		
	}
	// Help variable
	if(!strcmp(var, "help")){
		ADHOC_INFO_ONLY = true;
		printf("[1mNAME[22m\n\tADHOC - Action-Driven Human-Oriented Compiler\n\n");
		printf("[1mDESCRIPTION[22m\n\tParses a programming logic file and generates source code in a target\n\tlanguage. Expects a FILENAME (typically with extension '.adh') which\n\tcontains a representation of programming logic. If no FILENAME is\n\tprovided, ADHOC takes its input from stdin. So you can, for instance,\n\tsend logic content in via a pipe:\n\t\tcat foo.adh | adhoc -l c\n\t\t./logic_script.sh | adhoc -l sh -o other_script.sh\n\n");
		printf("[1mUSAGE SYNOPSIS[22m\n\tadhoc [ARGUMENT]... [FILENAME]\n\n");
		printf("[1mARGUMENTS[22m\n");
		printf("\t-c [1;4mfilename[22;24m, --config=[1;4mfilename[22;24m\n\t\tUse [1;4mfilename[22;24m as ADHOC's configuration file instead of the\n\t\tdefault adhoc.ini file.\n\n");
		printf("\t-h, --help\n\t\tPrint this usage information.\n\n");
		printf("\t-l [1;4mlang[22;24m, --language=[1;4mlang[22;24m\n\t\tSet the target language for code generation to [1;4mlang[22;24m. This\n\t\toverrides the value set for ADHOC_TARGET_LANGUAGE in the config\n\t\tfile.\n\n");
		printf("\t-o [1;4mfilename[22;24m, --outfile=[1;4mfilename[22;24m\n\t\tDirects generated target code to [1;4mfilename[22;24m instead of stdout.\n\n");
		printf("\t-v, --version\n\t\tPrint ADHOC version information.\n\n");
		return;
	}
	// Language variable
	if(!strcmp(var, "language")){
		memset(ADHOC_TARGET_LANGUAGE, 0, 30);
		strncpy(ADHOC_TARGET_LANGUAGE, val, 29);
	}
	// Outfile change variable
	if(!strcmp(var, "outfile")){
		if(!freopen(val, "w", stdout)){
			sprintf(errBuf, "Could not open file for writing: %-45s", val);
		}
	}
	// Version information variable
	if(!strcmp(var, "version")){
		ADHOC_INFO_ONLY = true;
		printf("[4;38;5;242mADHOC Version:[24;39m\n%s\n", ADHOC_VERSION_NUMBER);
		return;
	}
	sprintf(errBuf, "Unknown CLI variable: '--%-50s'\n", var);
}

// Functiont to handle command line flags
void adhoc_handleCLIFlag(char flag, char* val, char* errBuf){
	// Switch converts commandline flags into full length variables
	switch(flag){
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
	if(!strcmp(var, "ADHOC_TARGET_LANGUAGE")){
		strncpy(ADHOC_TARGET_LANGUAGE, val, 29);
		return;
	}
	if(!strcmp(var, "ADHOC_OUPUT_COLOR")){
		ADHOC_OUPUT_COLOR = (bool) strcmp(val, "true");
		return;
	}
	sprintf(errBuf, "Unknown config variable: %-40s\n", var);
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

// Initialize ADHOC. Read conficuration files / arguments
void adhoc_init(int argc, char** argv, char* errBuf){
	// In case a config file is not specified, use 'adhoc.ini'
	memset(ADHOC_CONFIG_LOCATION, 0, 100);
	strcpy(ADHOC_CONFIG_LOCATION, "adhoc.ini");

	// Handle commandline arguments
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

	// Load the config file
	FILE* conf;
	if(!(conf = fopen(ADHOC_CONFIG_LOCATION, "r"))){
		sprintf(errBuf, "Could not open config file: %-50s", ADHOC_CONFIG_LOCATION);
	}

	// Read in lines of the config file and process them
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
}

// Validate and optimize the abstract syntax tree
char* adhoc_validate(){
	adhoc_treeWalk(adhoc_printNode, ASTroot, 0);
	return NULL;
}

// Generate the target language code
char* adhoc_generate(){
// TODO: make some C!
//	ADHOC_TARGET_LANGUAGE
	return NULL;
}

// Clean up ADHOC
int adhoc_free(){
	hashMap_destroy(nodeMap, adhoc_destroyNode);
	hashMap_destroy(moduleMap, adhoc_destroyItemLocation);
	adhoc_destroyNode(readNode);
	return 0;
}

#endif
