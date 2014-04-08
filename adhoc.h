#ifndef ADHOC_H
#define ADHOC_H
#include <stdlib.h>
#include <stdbool.h>
#include "hashmap.h"
#include "adhoc_types.h"

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
bool ADHOC_OUPUT_COLOR = true;
hashMap_uint ADHOC_ESTIMATED_NODE_COUNT = 100;

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
}

// Walk the tree with a function to perform on each node
void adhoc_treeWalk(walk_func f, ASTnode* n, int d){
	f(n, d);
	int i;
	for(i=0; i<n->countChildren; ++i){
		adhoc_treeWalk(f, n->children[i], d+1);
	}
}

// Simple functions for hashing AST nodes
hashMap_uint adhoc_hashNode(void* n){
	return (hashMap_uint) ((ASTnode*) n)->id;
}
hashMap_uint adhoc_hashParent(void* n){
	return (hashMap_uint) ((ASTnode*) n)->parentId;
}

// Initialize ADHOC. Read conficuration files / arguments
char* adhoc_init(){
	nodeMap = hashMap_create(&adhoc_hashNode, ADHOC_ESTIMATED_NODE_COUNT);
	readNode = adhoc_createBlankNode();
	return NULL;
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
		parent->children = realloc(parent->children, parent->sizeChildren);
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
	return NULL;
}

// Clean up ADHOC
int adhoc_free(){
	hashMap_destroy(nodeMap, adhoc_destroyNode);
	adhoc_destroyNode(readNode);
	return 0;
}

#endif
