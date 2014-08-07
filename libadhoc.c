#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "hashmap.h"
#include "libadhoc.h"

// Create a referenced data struct
adhoc_data* adhoc_createData(adhoc_dataType t, void* d, adhoc_dataType c, int n){
	adhoc_data* ret = malloc(sizeof(adhoc_data));
	ret->refs = 0;
	ret->type = t;
	ret->data = d;
	ret->dataType = c;
	ret->countData = n;
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
		((adhoc_data**)d->data)[i] = (adhoc_data*)item;
		break;
	}
}

// Remove a reference to a referenced data struct and delete it if last
void adhoc_unassignData(adhoc_data* d){
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
				adhoc_unassignData(item);
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

// Format-print arguments
void adhoc_print(char* format, ...){
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}
