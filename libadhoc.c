#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "hashmap.h"
#include "libadhoc.h"

// Create a referenced data struct
adhoc_data* adhoc_createData(adhoc_dataType t, void* d, adhoc_dataType c, int n){
	adhoc_data* ret = malloc(sizeof(adhoc_data));
	ret->refs = 0;
	ret->type = t;
	ret->data = d;
	ret->dataType = c;
	ret->countData = 0;
	ret->sizeData = n;
	return ret;
}

// Add a reference to a referenced data struct
adhoc_data* adhoc_referenceData(adhoc_data* d){
	++d->refs;
	return d;
}

// Add an item to a referenced data array struct
void adhoc_assignArrayData(adhoc_data* arr, int i, void* item, float primVal){
	// If the new index is beyond the bounds of the array, grow the array
	int newSize = (arr->sizeData ? arr->sizeData : 1);
	while(i >= newSize) newSize *= 2;
	if(newSize > arr->sizeData){
		short s;
		switch(arr->dataType){
		case DATA_BOOL: s = sizeof(bool); break;
		case DATA_INT: s = sizeof(int); break;
		case DATA_FLOAT: s = sizeof(float); break;
		default:
			s = sizeof(adhoc_data*);
		}
		arr->data = realloc(arr->data, s*newSize);
		memset(arr->data+arr->sizeData, 0, (newSize - arr->sizeData)*s);
		arr->sizeData = newSize;
	}

	// Assign the new value to the index
	switch(arr->dataType){
	case DATA_VOID:
		break;
	case DATA_BOOL:
		((bool*)arr->data)[i] = (bool)primVal;
		break;
	case DATA_INT:
		((int*)arr->data)[i] = (int)primVal;
		break;
	case DATA_FLOAT:
		((float*)arr->data)[i] = (float)primVal;
		break;
	case DATA_STRING:
	case DATA_ARRAY:
	case DATA_HASH:
	case DATA_STRUCT:
		// If complex, remove a reference to the old item
		;adhoc_data** ptr = ((adhoc_data**)arr->data) + i;
		if(*ptr) adhoc_unreferenceData(*ptr);
		else ++arr->countData;
		*ptr = (adhoc_data*)item;

		// Add a reference to the new item
		adhoc_referenceData((adhoc_data*)item);
		break;
	}
}

// Remove a reference to a referenced data struct and delete it if last
void adhoc_unreferenceData(adhoc_data* d){
	if(!d || --d->refs > 0) return;
	int i, cleared;
	switch(d->type){
	case DATA_VOID:
	case DATA_BOOL:
	case DATA_INT:
	case DATA_FLOAT:
		break;
	case DATA_STRING:
		free(d->data);
		break;
	case DATA_ARRAY:
		switch(d->dataType){
		case DATA_VOID:
		case DATA_BOOL:
		case DATA_INT:
		case DATA_FLOAT:
			break;
		default:
			for(i=0,cleared=0; i<d->sizeData&&cleared<d->countData; ++i){
				adhoc_data* item = ((adhoc_data**)d->data)[i];
				if(!item) continue;
				adhoc_unreferenceData(item);
				++cleared;
			}
		}
		free(d->data);
		break;
	case DATA_HASH:
		break;
	case DATA_STRUCT:
		break;
	}
	free(d);
}

// Get the data from a referenced data struct
void* adhoc_getData(adhoc_data* d){
	return d->data;
}

// Get the simple data at a particular index of an array
float adhoc_getSArrayData(adhoc_data* arr, int i){
	// If the index is beyond the bounds of the array, return 0
	if(i >= arr->sizeData) return 0;
	return ((float*)arr->data)[i];
}

// Get the complex data at a particular index of an array
adhoc_data* adhoc_getCArrayData(adhoc_data* arr, int i){
	// If the index is beyond the bounds of the array, return 0
	if(i >= arr->sizeData) return NULL;
	return ((adhoc_data**)arr->data)[i];
}

// Create a new string and return its reference
adhoc_data* adhoc_createString(char* s){
	return adhoc_createData(
		DATA_STRING
		,strcpy(malloc(strlen(s)+1), s)
		,DATA_VOID
		,0
	);
}

// Create a new array and return its reference
adhoc_data* adhoc_createArray(adhoc_dataType t, int n){
	short s;
	switch(t){
	case DATA_BOOL: s = sizeof(bool); break;
	case DATA_INT: s = sizeof(int); break;
	case DATA_FLOAT: s = sizeof(float); break;
	default:
		s = sizeof(adhoc_data*);
	}
	return adhoc_createData(
		DATA_ARRAY
		,calloc(n, s)
		,t
		,n
	);
}

// Format-print arguments
void adhoc_print(char* format, ...){
	// Initialize arguments
	bool end;
	char* prcnt, buf[10];
	va_list args;
	va_start(args, format);

	// Iterate through the format string
	end = !(*format);
	while(!end){
		// Find the next format
		prcnt = strchr(format+1, '%');
		if(prcnt){
			strncpy(buf, format, prcnt-format);
			buf[prcnt-format] = 0;
		}else{
			strcpy(buf, format);
			end = true;
		}

		// Print the next argument
		switch(buf[strlen(buf)-1]){
		case 'd':
			// Fetch a primative
			printf("%d", va_arg(args, int));
			break;
		case 'f':
			// Fetch a primative
			printf("%f", va_arg(args, double));
			break;
		case 's':;
			// Fetch a complex item
			adhoc_data* d = va_arg(args, adhoc_data*);
			adhoc_referenceData(d);
			printf("%s", (char*)(d->data));
			adhoc_unreferenceData(d);
			break;
		default:
			// Handle other cases
			va_arg(args, void*);
			break;
		}
		format = prcnt;
	}

	// End the argument lists
	va_end(args);
}
