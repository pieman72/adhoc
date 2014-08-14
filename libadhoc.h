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
} adhoc_data;

// Create a referenced data struct
adhoc_data* adhoc_createData(adhoc_dataType t, void* d, adhoc_dataType c, int n);

// Add a reference to a referenced data struct
adhoc_data* adhoc_referenceData(adhoc_data* d);

// Add an item to a referenced data array struct
void adhoc_assignArrayData(adhoc_data* d, int i, void* item, float primVal);

// Remove a reference to a referenced data struct and delete it if last
void adhoc_unreferenceData(adhoc_data* d);

// Get the data from a referenced data struct
void* adhoc_getData(adhoc_data* d);

// Get the simple data at a particular index of an array
float adhoc_getSArrayData(adhoc_data* d, int i);

// Get the complex data at a particular index of an array
adhoc_data* adhoc_getCArrayData(adhoc_data* d, int i);

// Create a new string and return its reference
adhoc_data* adhoc_createString(char* s);

// Create a new array and return its reference
adhoc_data* adhoc_createArray(adhoc_dataType t, int n);

// Format and print arguments
void adhoc_print(char*, ...);

#endif
