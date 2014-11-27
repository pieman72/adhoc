#ifndef LIB_ADHOC_H
#define LIB_ADHOC_H

// Enumeration of ADHOC-approved data types
typedef enum adhoc_dataType {
	DATA_VOID
	,DATA_BOOL
	,DATA_INT
	,DATA_FLOAT
	,DATA_STRING
	,DATA_ARRAY
	,DATA_HASH
	,DATA_STRUCT
} adhoc_dataType;

// Struct to hold data and references
typedef struct adhoc_data {
	int refs;
	adhoc_dataType type;
	void* data;
	adhoc_dataType dataType;
	int countData;
	int sizeData;
	char* mappedData;
} adhoc_data;


//-----------------------//
//    Data Allocation    //
//-----------------------//

// Create a referenced data struct
adhoc_data* adhoc_createData(adhoc_dataType t, void* d, adhoc_dataType c, int n);

// Create a new string and return its reference
adhoc_data* adhoc_createString(char* s);

// Create a new array and return its reference
adhoc_data* adhoc_createArray(adhoc_dataType t, int n);


//--------------------------//
//    Reference Counting    //
//--------------------------//

// Add a reference to a referenced data struct
adhoc_data* adhoc_referenceData(adhoc_data* d);

// Add an item to a referenced data array struct
void adhoc_assignArrayData(adhoc_data* arr, int i, void* item, float primVal);

// Remove a reference to a referenced data struct and delete it if last
adhoc_data* adhoc_unreferenceData(adhoc_data* d);


//------------------------------//
//    Accessing Complex Data    //
//------------------------------//

// Get the data from a referenced data struct
void* adhoc_getData(adhoc_data* d);

// Get the simple data at a particular index of an array
void* adhoc_getSArrayData(adhoc_data* arr, int i);

// Get the complex data at a particular index of an array
adhoc_data* adhoc_getCArrayData(adhoc_data* arr, int i);


//------------------------------//
//    Library API Functionss    //
//------------------------------//

// Returns the type (as an integer 0-9) of one complex argument
adhoc_dataType adhoc_type(adhoc_data* d);

// Returns the size (in bytes) of one simple argument (sizeofFloat)
int adhoc_sizeS(float d);

// Returns the size (in bytes) of one complex argument's data array
int adhoc_sizeC(adhoc_data* d);

// Returns the count of items in one simple argument (always 1)
int adhoc_countS(float d);

// Returns the count of mapped items in one complex argument's data array
int adhoc_countC(adhoc_data* d);

// Convert any simple datatype to a wrapped string
adhoc_data* adhoc_toStringS(adhoc_dataType t, float d);

// Convert any wrapped datatype to a wrapped string
adhoc_data* adhoc_toStringC(adhoc_data* d);

// Format-print arguments
void adhoc_print(char* format, ...);

// Propt for type-verified input for a variable
void adhoc_prompt(adhoc_dataType t, void* v);

// Append one argument to an existing string
void adhoc_append_to_string(char* format, adhoc_data* s, ...);

// Concatenate arbitrary arguments into one string
adhoc_data* adhoc_concat(char* format, ...);

#endif
