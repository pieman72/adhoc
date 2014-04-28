#ifndef ADHOC_TYPES_H
#define ADHOC_TYPES_H

// Types of AST nodes
typedef enum adhoc_nodeType {
	TYPE_NULL	// 0
	,ACTION  	// 1
	,GROUP   	// 2
	,CONTROL 	// 3
	,OPERATOR	// 4
	,ASSIGNMENT	// 5
	,VARIABLE	// 6
	,LITERAL 	// 7
} nodeType;

// Specific node type instances
typedef enum adhoc_nodeWhich {
	WHICH_NULL			// 0
	,ACTION_DEFIN		// 1
	,ACTION_CALL		// 2
	,GROUP_SERIAL		// 3
	,CONTROL_IF			// 4
	,CONTROL_LOOP		// 5
	,CONTROL_SWITCH		// 6
	,CONTROL_CASE		// 7
	,CONTROL_FORK		// 8
	,CONTROL_CNTNU		// 9
	,CONTROL_BREAK		// 10
	,CONTROL_RETRN		// 11
	,OPERATOR_PLUS		// 12
	,OPERATOR_MINUS		// 13
	,OPERATOR_TIMES		// 14
	,OPERATOR_DIVBY		// 15
	,OPERATOR_MOD		// 16
	,OPERATOR_EXP		// 17
	,OPERATOR_OR		// 18
	,OPERATOR_AND		// 19
	,OPERATOR_NOT		// 20
	,OPERATOR_EQUIV		// 21
	,OPERATOR_GRTTN		// 22
	,OPERATOR_LESTN		// 23
	,OPERATOR_GRTEQ		// 24
	,OPERATOR_LESEQ		// 25
	,OPERATOR_NOTEQ		// 26
	,OPERATOR_ARIND		// 27
	,OPERATOR_TRNIF		// 28
	,ASSIGNMENT_INCPR	// 29
	,ASSIGNMENT_INCPS	// 30
	,ASSIGNMENT_DECPR	// 31
	,ASSIGNMENT_DECPS	// 32
	,ASSIGNMENT_NEGPR	// 33
	,ASSIGNMENT_NEGPS	// 34
	,ASSIGNMENT_EQUAL	// 35
	,ASSIGNMENT_PLUS	// 36
	,ASSIGNMENT_MINUS	// 37
	,ASSIGNMENT_TIMES	// 38
	,ASSIGNMENT_DIVBY	// 39
	,ASSIGNMENT_MOD		// 40
	,ASSIGNMENT_EXP		// 41
	,ASSIGNMENT_OR		// 42
	,ASSIGNMENT_AND		// 43
	,VARIABLE_ASIGN		// 44
	,VARIABLE_EVAL		// 45
	,LITERAL_BOOL		// 46
	,LITERAL_INT		// 47
	,LITERAL_FLOAT		// 48
	,LITERAL_STRNG		// 49
	,LITERAL_ARRAY		// 50
	,LITERAL_HASH		// 51
	,LITERAL_STRCT		// 52
} nodeWhich;

// Child node connection types
typedef enum adhoc_nodeChildType {
	CHILD_NULL		// 0
	,STATEMENT		// 1
	,EXPRESSION		// 2
	,INITIALIZATION	// 3
	,CONDITION		// 4
	,CASE			// 5
	,PARAMETER		// 6
	,ARGUMENT		// 7
	,PARENT			// 8
	,CHILD			// 9
} nodeChildType;

// Data types
typedef enum adhoc_dataType {
	TYPE_VOID	// 0
	,TYPE_BOOL	// 1
	,TYPE_INT	// 2
	,TYPE_FLOAT	// 3
	,TYPE_STRNG	// 4
	,TYPE_ARRAY	// 5
	,TYPE_HASH	// 6
	,TYPE_STRCT	// 7
	,TYPE_ACTN	// 8
} dataType;

// String names for node types
const char* adhoc_nodeType_names[] = {
	"NULL"
	,"action"
	,"group"
	,"control"
	,"operator"
	,"assignment"
	,"variable"
	,"literal"
};

// String names for which nodes
const char* adhoc_nodeWhich_names[] = {
	"NULL"
	,"definition"
	,"call"
	,"serial"
	,"if"
	,"loop"
	,"switch"
	,"case"
	,"fork"
	,"continue"
	,"break"
	,"return"
	,"+"
	,"-"
	,"*"
	,"/"
	,"%"
	,"^"
	,"||"
	,"&&"
	,"!"
	,"=="
	,">"
	,"<"
	,">="
	,"<="
	,"!="
	,"[]"
	,"?:"
	,"++"
	,"++"
	,"--"
	,"--"
	,"!!"
	,"!!"
	,"="
	,"+="
	,"-="
	,"*="
	,"/="
	,"%="
	,"^="
	,"||="
	,"&&="
	,""
	,""
	,""
	,""
	,""
	,""
	,""
	,""
	,""
};

// String names for node child types
const char* adhoc_nodeChildType_names[] = {
	"NULL"
	,""
	,""
	,"initialization"
	,"condition"
	,"case"
	,"parameter"
	,""
	,"parent"
	,"child"
};

// An abstract syntax tree node for use during parsing
typedef struct ASTnode {
	int id;
	int parentId;
	struct ASTnode* parent;
	struct ASTnode* scope;
	nodeType nodeType;
	nodeWhich which;
	nodeChildType childType;
	dataType dataType;
	bool defined;
	char* package;
	char* name;
	char* value;
	unsigned short countChildren;
	unsigned short sizeChildren;
	struct ASTnode** children;
	unsigned short countScopeVars;
	unsigned short sizeScopeVars;
	struct ASTnode** scopeVars;
} ASTnode;

// Allocate memory for a blank node
ASTnode* adhoc_createBlankNode(){
	ASTnode* ret = (ASTnode*) malloc(sizeof(ASTnode));
	ret->package = NULL;
	ret->name = NULL;
	ret->value = NULL;
	ret->dataType = TYPE_VOID;
	ret->defined = false;
	ret->countChildren = 0;
	ret->sizeChildren = 0;
	ret->children = NULL;
	ret->countScopeVars = 0;
	ret->sizeScopeVars = 0;
	ret->scopeVars = NULL;
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
	free(n->scopeVars);
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

// Generic function pointer type for walks of the abstract syntax tree
typedef void (*walk_func)(ASTnode*, int);

// Walk an AST with a function to perform on each node
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

#endif
