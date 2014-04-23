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
	,OPERATOR_PLUS		// 9
	,OPERATOR_MINUS		// 10
	,OPERATOR_TIMES		// 11
	,OPERATOR_DIVBY		// 12
	,OPERATOR_MOD		// 13
	,OPERATOR_EXP		// 14
	,OPERATOR_OR		// 15
	,OPERATOR_AND		// 16
	,OPERATOR_NOT		// 17
	,OPERATOR_EQUIV		// 18
	,OPERATOR_GRTTN		// 19
	,OPERATOR_LESTN		// 20
	,OPERATOR_GRTEQ		// 21
	,OPERATOR_LESEQ		// 22
	,OPERATOR_NOTEQ		// 23
	,OPERATOR_ARIND		// 24
	,OPERATOR_TRNIF		// 25
	,OPERATOR_INCPR		// 26
	,OPERATOR_INCPS		// 27
	,OPERATOR_DECPR		// 28
	,OPERATOR_DECPS		// 29
	,OPERATOR_NEGPR		// 30
	,OPERATOR_NEGPS		// 31
	,ASSIGNMENT_EQUAL	// 32
	,ASSIGNMENT_PLUS	// 33
	,ASSIGNMENT_MINUS	// 34
	,ASSIGNMENT_TIMES	// 35
	,ASSIGNMENT_DIVBY	// 36
	,ASSIGNMENT_MOD		// 37
	,ASSIGNMENT_EXP		// 38
	,ASSIGNMENT_OR		// 39
	,ASSIGNMENT_AND		// 40
	,VARIABLE_ASIGN		// 41
	,VARIABLE_EVAL		// 42
	,LITERAL_BOOL		// 43
	,LITERAL_INT		// 44
	,LITERAL_FLOAT		// 45
	,LITERAL_STRNG		// 46
	,LITERAL_ARRAY		// 47
	,LITERAL_HASH		// 48
	,LITERAL_STRCT		// 49
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
	,"++_"
	,"_++"
	,"--_"
	,"_--"
	,"!!_"
	,"_!!"
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

#endif
