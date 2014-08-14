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
void adhoc_assignArrayData(adhoc_data* d, int i, void* item, float primVal){
	// If the new index is beyond the bounds of the array, grow the array
	int newSize = (d->sizeData ? d->sizeData : 1);
	while(i >= newSize) newSize *= 2;
	if(newSize > d->sizeData){
		short s;
		switch(d->dataType){
		case DATA_BOOL: s = sizeof(bool); break;
		case DATA_INT: s = sizeof(int); break;
		case DATA_FLOAT: s = sizeof(float); break;
		default:
			s = sizeof(adhoc_data*);
		}
		d->data = realloc(d->data, s*newSize);
		memset(d->data+d->sizeData, 0, (newSize - d->sizeData)*s);
		d->sizeData = newSize;
	}

	// Assign the new value to the index
	switch(d->dataType){
	case DATA_VOID:
		break;
	case DATA_BOOL:
		((bool*)d->data)[i] = (bool)primVal;
		break;
	case DATA_INT:
		((int*)d->data)[i] = (int)primVal;
		break;
	case DATA_FLOAT:
		((float*)d->data)[i] = (float)primVal;
		break;
	case DATA_STRING:
	case DATA_ARRAY:
	case DATA_HASH:
	case DATA_STRUCT:
		// If complex, remove a reference to the old item
		;adhoc_data** ptr = ((adhoc_data**)d->data) + i;
		if(*ptr) adhoc_unreferenceData(*ptr);
		else ++d->countData;
		*ptr = (adhoc_data*)item;
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
		// TODO
		break;
	case DATA_STRUCT:
		// TODO
		break;
	}
	free(d);
}

// Get the data from a referenced data struct
void* adhoc_getData(adhoc_data* d){
	return d->data;
}

// Get the simple data at a particular index of an array
float adhoc_getSArrayData(adhoc_data* d, int i){
	// If the index is beyond the bounds of the array, return 0
	if(i >= d->sizeData) return 0;
	return ((float*)d->data)[i];
}

// Get the complex data at a particular index of an array
adhoc_data* adhoc_getCArrayData(adhoc_data* d, int i){
	// If the index is beyond the bounds of the array, return 0
	if(i >= d->sizeData) return NULL;
	return ((adhoc_data**)d->data)[i];
}

// Create a new string and return its reference
adhoc_data* adhoc_createString(char* s){
	return adhoc_referenceData(adhoc_createData(
		DATA_STRING
		,strcpy(malloc(strlen(s)+1), s)
		,DATA_VOID
		,0
	));
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
	return adhoc_referenceData(adhoc_createData(
		DATA_ARRAY
		,calloc(n, s)
		,t
		,n
	));
}

// Format-print arguments
void adhoc_print(char* format, ...){
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
