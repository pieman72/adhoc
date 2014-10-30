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
	,INDEX			// 10
	,IF				// 11
	,ELSE			// 12
	,STORAGE		// 13
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
	,TYPE_MIXED	// 9
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
	,"index"
	,"if"
	,"else"
	,"storage"
};

// String names for data types
const char* adhoc_dataType_names[] = {
	"void"
	,"bool"
	,"int"
	,"float"
	,"char*"
	,"<<ARRAY>>"
	,"<<HASH>>"
	,"<<STRUCT>>"
	,"<<ACTION>>"
	,"mixed"
};

// Default values for data types
const char* adhoc_dataType_defaults[] = {
	"NULL"
	,"false"
	,"0"
	,"0"
	,"\"\""
	,"NULL"
	,"NULL"
	,"NULL"
	,"NULL"
	,"NULL"
};

// An abstract syntax tree node for use during parsing
typedef struct ASTnode {
	int id;
	int parentId;
	int refId;
	struct ASTnode* parent;
	struct ASTnode* scope;
	struct ASTnode* reference;
	nodeType nodeType;
	nodeWhich which;
	nodeChildType childType;
	dataType dataType;
	dataType childDataType;
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
	unsigned short countCmplxVals;
	unsigned short sizeCmplxVals;
	struct ASTnode** cmplxVals;
} ASTnode;

// Allocate memory for a blank node
ASTnode* adhoc_createBlankNode(){
	ASTnode* ret = (ASTnode*) malloc(sizeof(ASTnode));
	ret->id = 0;
	ret->parentId = 0;
	ret->refId = 0;
	ret->parent = NULL;
	ret->scope = NULL;
	ret->reference = NULL;
	ret->nodeType = TYPE_NULL;
	ret->which = WHICH_NULL;
	ret->childType = CHILD_NULL;
	ret->dataType = TYPE_VOID;
	ret->childDataType = TYPE_VOID;
	ret->defined = false;
	ret->package = NULL;
	ret->name = NULL;
	ret->value = NULL;
	ret->countChildren = 0;
	ret->sizeChildren = 0;
	ret->children = NULL;
	ret->countScopeVars = 0;
	ret->sizeScopeVars = 0;
	ret->scopeVars = NULL;
	ret->countCmplxVals = 0;
	ret->sizeCmplxVals = 0;
	ret->cmplxVals = NULL;
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
	free(n->cmplxVals);
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
typedef void (*walk_func)(ASTnode*, int, char*);

// Walk an AST with a function to perform on each node
void adhoc_treeWalk(walk_func f, ASTnode* n, int d, char* errBuf){
	f(n, d, errBuf);
	if(strlen(errBuf)) return;
	int i;
	for(i=0; i<n->countChildren; ++i){
		adhoc_treeWalk(f, n->children[i], d+1, errBuf);
	}
}

// Walk an AST in post-order with a function to perform on each node
void adhoc_treePostWalk(walk_func f, ASTnode* n, int d, char* errBuf){
	int i;
	for(i=0; i<n->countChildren; ++i){
		adhoc_treePostWalk(f, n->children[i], d+1, errBuf);
		if(strlen(errBuf)) return;
	}
	f(n, d, errBuf);
}

// Simple functions for hashing AST nodes
hashMap_uint adhoc_hashNode(void* n){
	return (hashMap_uint) ((ASTnode*) n)->id;
}
hashMap_uint adhoc_hashParent(void* n){
	return (hashMap_uint) ((ASTnode*) n)->parentId;
}

// Resolve compared types for implicit cast
dataType adhoc_resolveTypes(dataType a, dataType b){
	// Handle edge cases
	if(a==TYPE_MIXED || b==TYPE_MIXED) return TYPE_VOID;
	if(a==TYPE_ACTN || b==TYPE_ACTN) return TYPE_VOID;
	if(a==TYPE_STRCT || b==TYPE_STRCT) return TYPE_VOID;
	if(a==TYPE_VOID || b==TYPE_VOID) return TYPE_VOID;
	if(a==b) return a;
	if(a < b) return adhoc_resolveTypes(b, a);
	if(a==TYPE_HASH && b==TYPE_ARRAY) return TYPE_HASH;
	if(a==TYPE_HASH) return TYPE_VOID;
	if(a==TYPE_ARRAY) return TYPE_VOID;

	// Handle casts
	return a;
}

// Find the scope where a variable name v was first defined above scope s
ASTnode* adhoc_findScope(char* v, ASTnode* s){
	int i;
	while(1){
		for(i=0; i<s->countScopeVars; ++i){
			if(!strcmp(v, s->scopeVars[i]->name)) return s->scopeVars[i];
		}
		if(!s->scope) return NULL;
		s = s->scope;
	}
}

// Assigns scope of n to s
void adhoc_assignScope(ASTnode* n, ASTnode* s){
	// See if the scope is a reference to another
	if(n->nodeType == VARIABLE){
		ASTnode* ref = adhoc_findScope(n->name, s);
		if(ref){
			n->reference = ref;
			n->scope = ref->scope;
			return;
		}
	}

	// Set the scope pointer in v
	n->scope = s;

	// No need to define twice
	if(n->defined) return;
	n->defined = true;

	// If complex, assign to nearest statement ancestor
	if(n->nodeType == LITERAL && (
			n->dataType==TYPE_ARRAY
			|| n->dataType==TYPE_HASH
			|| n->dataType==TYPE_STRCT
		)){
		ASTnode* stmt = n->parent;
		while(stmt->childType != STATEMENT) stmt = stmt->parent;
		// If stmt has no cmplxVals, allocate the cmplxVals array
		if(!stmt->countCmplxVals){
			stmt->cmplxVals = malloc(sizeof(ASTnode*));
			stmt->sizeCmplxVals = 1;
		// If stmt is full of cmplxVals, double the cmplxVals array
		}else if(stmt->countCmplxVals == stmt->sizeCmplxVals){
			stmt->sizeCmplxVals *= 2;
			stmt->cmplxVals = realloc(stmt->cmplxVals, stmt->sizeCmplxVals*sizeof(ASTnode*));
		}
		// Add the node to stmt
		stmt->cmplxVals[stmt->countCmplxVals++] = n;
	}

	// Only certain types need to be added to the parent
	if(	   n->which != CONTROL_RETRN
		&& n->which != VARIABLE_ASIGN
		&& n->which != LITERAL_STRNG
		&& n->which != LITERAL_ARRAY
		&& n->which != LITERAL_HASH
		&& n->which != LITERAL_STRCT
	) return;

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
	s->scopeVars[s->countScopeVars++] = n;
}

// Track errors in order to report them
ASTnode* adhoc_errorNode;

#endif
