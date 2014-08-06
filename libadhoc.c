#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include "hashmap.h"
#include "libadhoc.h"

// Create a referenced data struct
adhoc_data* adhoc_createData(void* d){
	adhoc_data* ret = malloc(sizeof(adhoc_data));
	ret->refs = 0;
	ret->data = d;
	return ret;
}

// Add a reference to a referenced data struct
adhoc_data* adhoc_assignData(adhoc_data* d){
	++d->refs;
	return d;
}

// Remove a reference to a referenced data struct and delete it if last
void adhoc_unassignData(adhoc_data* d){
	if(!d || --d->refs>0) return;
	free(d->data);
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
