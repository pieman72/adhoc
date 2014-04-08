#ifndef ADHOC_TYPES_H
#define ADHOC_TYPES_H

// Types of AST nodes
typedef enum adhoc_nodeType {
	TYPE_NULL
	,ACTION
	,GROUP
	,CONTROL
	,OPERATOR
	,ASSIGNMENT
	,VARIABLE
	,LITERAL
} nodeType;

// Specific node type instances
typedef enum adhoc_nodeWhich {
	WHICH_NULL
	,ACTION_DEFIN
	,ACTION_CALL
	,GROUP_SERIAL
	,CONTROL_IF
	,CONTROL_LOOP
	,CONTROL_SWITCH
	,CONTROL_CASE
	,CONTROL_FORK
	,OPERATOR_PLUS
	,OPERATOR_MINUS
	,OPERATOR_TIMES
	,OPERATOR_DIVBY
	,OPERATOR_MOD
	,OPERATOR_EXP
	,OPERATOR_OR
	,OPERATOR_AND
	,OPERATOR_NOT
	,OPERATOR_EQUIV
	,OPERATOR_GRTTN
	,OPERATOR_LESTN
	,OPERATOR_GRTEQ
	,OPERATOR_LESEQ
	,OPERATOR_NOTEQ
	,OPERATOR_ARIND
	,OPERATOR_TRNIF
	,OPERATOR_INCPR
	,OPERATOR_INCPS
	,OPERATOR_DECPR
	,OPERATOR_DECPS
	,OPERATOR_NEGPR
	,OPERATOR_NEGPS
	,ASSIGNMENT_EQUAL
	,ASSIGNMENT_PLUS
	,ASSIGNMENT_MINUS
	,ASSIGNMENT_TIMES
	,ASSIGNMENT_DIVBY
	,ASSIGNMENT_MOD
	,ASSIGNMENT_EXP
	,ASSIGNMENT_OR
	,ASSIGNMENT_AND
	,VARIABLE_ASIGN
	,VARIABLE_EVAL
	,LITERAL_BOOL
	,LITERAL_INT
	,LITERAL_FLOAT
	,LITERAL_STRNG
	,LITERAL_ARRAY
	,LITERAL_HASH
	,LITERAL_STRCT
} nodeWhich;

// Child node connection types
typedef enum adhoc_nodeChildType {
	CHILD_NULL
	,STATEMENT
	,EXPRESSION
	,CONDITION
	,INITIALIZATION
	,CASE
	,PARAMETER
	,ARGUMENT
	,PARENT
	,CHILD
} nodeChildType;

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
	,"condition"
	,"initialization"
	,"case"
	,"parameter"
	,""
	,"parent"
	,"child"
};

#endif
