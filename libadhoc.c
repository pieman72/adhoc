#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include "hashmap.h"
#include "libadhoc.h"

const ushort DATA_MAP_BIT_FIELD_SIZE = 8;


//-----------------------//
//    Data Allocation    //
//-----------------------//

// Create a referenced data struct
adhoc_data* adhoc_createData(adhoc_dataType t, void* d, adhoc_dataType c, int n){
	adhoc_data* ret = malloc(sizeof(adhoc_data));
	ret->refs = 0;
	ret->type = t;
	ret->data = d;
	ret->dataType = c;
	ret->countData = 0;
	ret->sizeData = n;
	if(t == DATA_ARRAY){
		int size = (n-1)/DATA_MAP_BIT_FIELD_SIZE+1;
		ret->mappedData = calloc(size, sizeof(void*));
	}else{
		ret->mappedData = NULL;
	}
	return ret;
}

// Create a new string and return its reference
adhoc_data* adhoc_createString(char* s){
	return adhoc_createData(
		DATA_STRING
		,strcpy(malloc(strlen(s)+1), s)
		,DATA_VOID
		,strlen(s)+1
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


//--------------------------//
//    Reference Counting    //
//--------------------------//

// Add a reference to a referenced data struct
adhoc_data* adhoc_referenceData(adhoc_data* d){
	++d->refs;
	return d;
}

// Add an item to a referenced data array struct
void adhoc_assignArrayData(adhoc_data* arr, int i, void* item, float primVal){
	adhoc_referenceData(arr);

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
		memset(arr->data+(arr->sizeData*s), 0, (newSize - arr->sizeData)*s);
		int oldMapSize = (arr->sizeData-1)/DATA_MAP_BIT_FIELD_SIZE+1;
		int newMapSize = (newSize-1)/DATA_MAP_BIT_FIELD_SIZE+1;
		arr->mappedData = realloc(arr->mappedData, newMapSize);
		memset(arr->mappedData+oldMapSize, 0, newMapSize-oldMapSize);
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

		// Add a reference to the new item
		*ptr = (adhoc_data*)item;
		adhoc_referenceData(*ptr);
		break;
	}
	// If the new index was not previously mapped, increment the count and map
	if(!(arr->mappedData[i/DATA_MAP_BIT_FIELD_SIZE]
			& (1<<(i%DATA_MAP_BIT_FIELD_SIZE))
		)){
		++arr->countData;
		*(arr->mappedData+i/DATA_MAP_BIT_FIELD_SIZE)
			|= (1<<(i%DATA_MAP_BIT_FIELD_SIZE));
	}

	adhoc_unreferenceData(arr);
}

// Remove a reference to a referenced data struct and delete it if last
adhoc_data* adhoc_unreferenceData(adhoc_data* d){
	if(!d || --d->refs > 0) return d;
	int i, cleared;
	switch(d->type){
	case DATA_VOID:
	case DATA_BOOL:
	case DATA_INT:
	case DATA_FLOAT:
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
		free(d->mappedData);
		break;
	case DATA_HASH:
		// TODO
		break;
	case DATA_STRUCT:
		// TODO
		break;
	}
	free(d);
	return NULL;
}


//------------------------------//
//    Accessing Complex Data    //
//------------------------------//

// Get the data from a referenced data struct
void* adhoc_getData(adhoc_data* d){
	return d->data;
}

// Get the simple data at a particular index of an array
void* adhoc_getSArrayData(adhoc_data* arr, int i){
	// TODO: Throw warning instead of returning 0
	// If the index is beyond the bounds of the array, return 0
	if(i >= arr->sizeData) return NULL;
	// If the field is unset, return 0
	if(!(arr->mappedData[i/DATA_MAP_BIT_FIELD_SIZE]
			& (1<<(i%DATA_MAP_BIT_FIELD_SIZE)))
		) return NULL;
	switch(arr->dataType){
	case DATA_VOID:
		return arr->data+i;
	case DATA_BOOL:
		return (void*)(((bool*)arr->data)+i);
	case DATA_INT:
		return (void*)(((int*)arr->data)+i);
	case DATA_FLOAT:
		return (void*)(((float*)arr->data)+i);
	default:
		return NULL;
	}
}

// Get the complex data at a particular index of an array
adhoc_data* adhoc_getCArrayData(adhoc_data* arr, int i){
	// TODO: Throw warning instead of returning 0
	// If the index is beyond the bounds of the array, return NULL
	if(i >= arr->sizeData) return NULL;
	// If the field is unset, return NULL
	if(!(arr->mappedData[i/DATA_MAP_BIT_FIELD_SIZE]
			& (1<<(i%DATA_MAP_BIT_FIELD_SIZE))))
		return NULL;
	return ((adhoc_data**)arr->data)[i];
}


//------------------------------//
//    Library API Functionss    //
//------------------------------//

//-- GENERAL --//
// Returns the type (as an integer 0-9) of one complex argument
adhoc_dataType adhoc_type(adhoc_data* item){
	return item->dataType;
}

// Returns the size (in bytes) of one simple argument (sizeofFloat)
int adhoc_sizeS(float item){
	return sizeof(item);
}

// Returns the size (in bytes) of one complex argument's data array
int adhoc_sizeC(adhoc_data* item){
	adhoc_referenceData(item);
	int ret = item->sizeData - (item->type==DATA_STRING?1:0);
	adhoc_unreferenceData(item);
	return ret;
}

// Returns the count of items in one simple argument (always 1)
int adhoc_countS(float item){
	return sizeof(item)/sizeof(float);
}

// Returns the count of mapped items in one complex argument's data array
int adhoc_countC(adhoc_data* item){
	return (item->type==DATA_STRING) ? 1 : item->countData;
}

// Convert any simple datatype to a wrapped string
adhoc_data* adhoc_toStringS(adhoc_dataType t, float item){
	// Wrap the simple data and pass it to adhoc_toStringC
	void* itemp;
	switch(t){
	case DATA_BOOL: *((bool*)(itemp = malloc(sizeof(bool)))) = (bool)item; break;
	case DATA_INT: *((int*)(itemp = malloc(sizeof(int)))) = (int)item; break;
	case DATA_FLOAT: *((float*)(itemp = malloc(sizeof(float)))) = item; break;
	default: itemp = NULL;
	}
	return adhoc_toStringC(adhoc_createData(t, itemp, DATA_VOID, 1));
}

// Convert any wrapped datatype to a wrapped string
adhoc_data* adhoc_toStringC(adhoc_data* item){
	adhoc_referenceData(item);

	// Create an output buffer
	int size = 256,len;
	char* buf = malloc(size);
	buf[0] = '\0';

	// Depending on the datatype, we'll add different things to the buffer
	switch(item->type){
	case DATA_VOID:
		snprintf(buf, size, "<<VOID>>"); break;
	case DATA_BOOL:
		snprintf(buf, size, "%s", *((bool*)item->data)?"true":"false"); break;
	case DATA_INT:
		snprintf(buf, size, "%d", *((int*)item->data)); break;
	case DATA_FLOAT:
		snprintf(buf, size, "%f", *((float*)item->data)); break;
	// Unwrap strings
	case DATA_STRING:;
		len = item->sizeData;
		if(size<len) buf = realloc(buf, size=len);
		memcpy(buf, item->data, len);
		break;
	// Handle arrays
	case DATA_ARRAY:
		;int i=0
			,checked=0
			,total=1;
		adhoc_data* str=NULL;
		buf[0] = '[';
		buf[1] = '\0';
		// Loop through array contents and print differently depending on types
		for(i=0,checked=0; i<item->sizeData && checked<item->countData; ++i){
			if(!(item->mappedData[i/DATA_MAP_BIT_FIELD_SIZE]
					& (1<<(i%DATA_MAP_BIT_FIELD_SIZE))))
				continue;

			switch(item->dataType){
			case DATA_VOID:
				str = adhoc_referenceData(adhoc_createString("<<VOID>>"));
				break;
			case DATA_BOOL:
				str = adhoc_referenceData(adhoc_toStringS(
					DATA_BOOL
					,*((bool*)adhoc_getSArrayData(item, i))
				));
				break;
			case DATA_INT:
				str = adhoc_referenceData(adhoc_toStringS(
					DATA_INT
					,*((int*)adhoc_getSArrayData(item, i))
				));
				break;
			case DATA_FLOAT:
				str = adhoc_referenceData(adhoc_toStringS(
					DATA_FLOAT
					,*((float*)adhoc_getSArrayData(item, i))
				));
				break;
			case DATA_STRING:
			case DATA_ARRAY:
			case DATA_HASH:
			case DATA_STRUCT:
				// Get one item in the array
				;adhoc_data* cItem = adhoc_getCArrayData(item, i);
				str = adhoc_referenceData(adhoc_toStringC(cItem));
				break;
			}

			// Add commas
			if(checked){
				while(size<total+3) buf = realloc(buf,(size*=2));
				buf[total++] = ',';
				buf[total++] = ' ';
				buf[total] = '\0';
			}

			// Convert that item to a string object, and add it to the output
			if(str){
				len = str->sizeData-1;
				while(size<total+len+1) buf = realloc(buf,(size*=2));
				strncpy(buf+total, (char*)str->data, size-total-1);
				total += len;
			}

			// Drop references
			adhoc_unreferenceData(str);
			++checked;
		}
		if(size<total+2) buf = realloc(buf,(size*=2));
		buf[total++]=']';
		buf[total]='\0';
		break;

	case DATA_HASH: snprintf(buf, size, "<<HASH>>"); break;
	case DATA_STRUCT: snprintf(buf, size, "<<STRUCT>>"); break;
	}

	adhoc_unreferenceData(item);
	adhoc_data* ret = adhoc_createString(buf);
	free(buf);
	return ret;
}

//-- I/O --//
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
		switch(buf[1]){
		case 'b':
			// Fetch a boolean primative
			printf("%s", va_arg(args, int) ? "true" : "false");
			break;
		case 'd':
			// Fetch an integer primative
			printf("%d", va_arg(args, int));
			break;
		case 'f':
			// Fetch a float primative
			printf("%f", va_arg(args, double));
			break;
		case 's':
			// Fetch a string
			;adhoc_data* str = adhoc_referenceData(va_arg(args, adhoc_data*));
			printf("%s", (char*)(str->data));
			adhoc_unreferenceData(str);
			break;
		case '_':
			// Fetch a complex item
			;adhoc_data* dat = adhoc_referenceData(va_arg(args, adhoc_data*));
			adhoc_data* datStr = adhoc_referenceData(adhoc_toStringC(dat));
			adhoc_unreferenceData(dat);
			printf("%s", (char*)adhoc_getData(datStr));
			adhoc_unreferenceData(datStr);
			break;
		default:
			// Handle other cases
			va_arg(args, void*);
			break;
		}
		printf("%s", buf+2);
		format = prcnt;
	}

	// End the argument lists
	va_end(args);
}

//-- STRINGS --//
// Append one argument to an existing string
void adhoc_append_to_string(char* format, adhoc_data* baseString, ...){
	adhoc_referenceData(baseString);

	// Initialize arguments
	adhoc_data* str = NULL;
	int len;
	char buf[20];
	va_list args;
	va_start(args, baseString);

	// Switch based on the argument type
	switch(format[1]){
	case 'b':
		// Fetch a boolean primative
		str = adhoc_referenceData(adhoc_createString(
			va_arg(args, int) ? "true" : "false")
		);
		break;
	case 'd':
		// Fetch an integer primative
		snprintf(buf, 20, "%d", va_arg(args, int));
		str = adhoc_referenceData(adhoc_createString(buf));
		break;
	case 'f':
		// Fetch a float primative
		snprintf(buf, 20, "%f", va_arg(args, double));
		str = adhoc_referenceData(adhoc_createString(buf));
		break;
	case 's':
		// Fetch a string
		str = adhoc_referenceData(va_arg(args, adhoc_data*));
		break;
	case '_':
		// Fetch a complex item
		;adhoc_data* dat = adhoc_referenceData(va_arg(args, adhoc_data*));
		str = adhoc_referenceData(adhoc_toStringC(dat));
		adhoc_unreferenceData(dat);
		break;
	default:
		// Handle other cases
		va_arg(args, void*);
		break;
	}

	// Append str to baseString
	len = baseString->sizeData-1 + str->sizeData;
	baseString->data = realloc(baseString->data, len);
	memcpy(baseString->data+baseString->sizeData-1, str->data, str->sizeData);
	baseString->sizeData = len;
	adhoc_unreferenceData(str);
	adhoc_unreferenceData(baseString);

	// End the argument lists
	va_end(args);
}

// Concatenate arbitrary arguments into one string
adhoc_data* adhoc_concat(char* format, ...){
	// Initialize arguments
	adhoc_data* str = adhoc_createString("");
	adhoc_data* newItem = NULL;
	int len = 1,
		newLen = 1,
		newSize = 1;
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

		// Stringify the next argument
		switch(buf[1]){
		case 'b':
			// Fetch a boolean primative
			newItem = adhoc_referenceData(adhoc_createString(
				va_arg(args, int) ? "true" : "false"
			));
			break;
		case 'd':
			// Fetch an integer primative
			newItem = adhoc_referenceData(adhoc_toStringS(DATA_INT,
				va_arg(args, int)
			));
			break;
		case 'f':
			// Fetch a float primative
			newItem = adhoc_referenceData(adhoc_toStringS(DATA_FLOAT,
				va_arg(args, double)
			));
			break;
		case 's':
			// Fetch a string
			newItem = adhoc_referenceData(va_arg(args, adhoc_data*));
			break;
		case '_':
			// Fetch a complex item
			;adhoc_data* dat = adhoc_referenceData(va_arg(args, adhoc_data*));
			newItem = adhoc_referenceData(adhoc_toStringC(dat));
			adhoc_unreferenceData(dat);
			break;
		default:
			// Handle other cases
			va_arg(args, void*);
			break;
		}

		// Copy into the output string
		newLen += newItem->sizeData-1;
		while(newSize < newLen) newSize <<= 1;
		if(newSize > str->sizeData)
			str->data = realloc(str->data, (str->sizeData=newSize));
		memcpy((char*)str->data+len-1, (char*)newItem->data, newLen-len+1);
		len = newLen;
		adhoc_unreferenceData(newItem);

		// Advance the format pointer
		format = prcnt;
	}

	// End the argument lists
	va_end(args);

	// Return the string with all the concatenations
	str->data = realloc(str->data, newLen);
	return str;
}

// Copy from s starting at start and running for length
adhoc_data* adhoc_substring(adhoc_data* baseString, int index, int length){
	adhoc_referenceData(baseString);
	++length;
	if(index>=baseString->sizeData || length<1){
		adhoc_unreferenceData(baseString);
		return adhoc_createString("");
	}
	if(index+length >= baseString->sizeData)
		length = baseString->sizeData - index;
	char* data = malloc(length);
	memcpy(data, baseString->data+index, length);
	data[length-1] = '\0';
	adhoc_unreferenceData(baseString);
	return adhoc_createData(DATA_STRING, data, DATA_VOID, length);
}

// Patch the replacement over the base string at index return what is replaced
adhoc_data* adhoc_splice_string(adhoc_data* baseString, adhoc_data* replacement, int index, int length){
	adhoc_referenceData(baseString);
	adhoc_referenceData(replacement);
	adhoc_data* ret = adhoc_substring(baseString, index, length);
	if(index >= baseString->sizeData){
		adhoc_unreferenceData(baseString);
		adhoc_unreferenceData(replacement);
		return ret;
	}
	int newLen = baseString->sizeData - ret->sizeData + replacement->sizeData;
	int carryOverLen = (index+length < baseString->sizeData-1)
		? baseString->sizeData-1 - (index+length)
		: 0;
	char* tempBuffer = NULL;
	if(carryOverLen){
		tempBuffer = memcpy(
			malloc(carryOverLen)
			,baseString->data+index+length
			,carryOverLen
		);
	}
	memcpy(
		baseString->data+index
		,replacement->data
		,replacement->sizeData-1
	);
	if(carryOverLen){
		memcpy(
			baseString->data+index+replacement->sizeData-1
			,tempBuffer
			,carryOverLen
		);
		free(tempBuffer);
	}
	baseString->sizeData = newLen;
	baseString->data = realloc(baseString->data, newLen);
	((char*)(baseString->data))[newLen-1] = '\0';
	adhoc_unreferenceData(baseString);
	adhoc_unreferenceData(replacement);
	return ret;
}

// Finds the first occurrence of targetsString in baseString
int adhoc_find_in_string(adhoc_data* baseString, adhoc_data* targetsString){
	adhoc_referenceData(baseString);
	adhoc_referenceData(targetsString);
	int ret = -1;
	char* loc = strstr((char*)baseString->data, (char*)targetsString->data);
	if(loc) ret = loc - (char*)baseString->data;
	adhoc_unreferenceData(baseString);
	adhoc_unreferenceData(targetsString);
	return ret;
}

//-- ARRAYS --//
// Append one item to an existing array
void adhoc_append_to_array(char* format, adhoc_data* baseArray, ...){
	adhoc_referenceData(baseArray);

	// Initialize arguments
	va_list args;
	va_start(args, baseArray);

	// Extend find the highest unused index

	int pos = 0;
	if(baseArray->sizeData){
		pos = baseArray->sizeData-1;
		while(!(baseArray->mappedData[pos/DATA_MAP_BIT_FIELD_SIZE]
				& (1<<(pos%DATA_MAP_BIT_FIELD_SIZE))
			)){
			--pos;
		}
		++pos;
	}

	// Switch based on the argument type
	switch(format[1]){
	case 'b':
		// Fetch a boolean primative
		adhoc_assignArrayData(baseArray, pos, NULL, va_arg(args, int));
		break;
	case 'd':
		// Fetch an integer primative
		adhoc_assignArrayData(baseArray, pos, NULL, va_arg(args, int));
		break;
	case 'f':
		// Fetch a float primative
		adhoc_assignArrayData(baseArray, pos, NULL, va_arg(args, double));
		break;
	case 's':
	case '_':
		// Fetch a string or other complex item
		adhoc_assignArrayData(baseArray, pos, va_arg(args, adhoc_data*), 0);
		break;
	default:
		// Handle other cases
		va_arg(args, void*);
		break;
	}

	// End the argument lists
	va_end(args);

	adhoc_unreferenceData(baseArray);
}
