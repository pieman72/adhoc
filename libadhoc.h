#ifndef LIB_ADHOC_H
#define LIB_ADHOC_H

// Struct to hold data and references
typedef struct adhoc_data {
	int refs;
	void* data;
} adhoc_data;
adhoc_data* adhoc_createData(void* d);
adhoc_data* adhoc_assignData(adhoc_data* d);
void adhoc_unassignData(adhoc_data* d);
void* adhoc_getData(adhoc_data* d);

void adhoc_print(char*, ...);

#endif
