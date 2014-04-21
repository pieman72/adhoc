#ifndef HASHMAP_H
#define HASHMAP_H

// Why hashMap_uint no standard??
typedef unsigned int hashMap_uint;

// A small struct to hold a hashed key, and a value pointer
typedef struct hashMapItem {
	hashMap_uint key;
	void* value;
} hashMapItem;

// A struct to store a hashMap
typedef struct hashMap {
	hashMap_uint size;
	int count;
	hashMapItem** items;
	hashMap_uint (*hash)(void*);
} hashMap;

// Generic function pointer types for user's hashing and destructor functions
typedef hashMap_uint (*hash_func)(void*);
typedef void (*destruct_func)(void*);

// Reasonable hashMap sizes (primes)
// Credit to: http://www.w3.org/2001/06/blindfold/api/hashMap_8c-source.html
const hashMap_uint HASHMAP_SIZES[] = {
	7, 13, 31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
	32749, 65521, 131071, 262143, 524287, 1048575, 2097151,
	4194303, 8388607, 16777211, 33554431, 67108863, 134217727,
	268435455, 536870911, 1073741823, 2147483647
};

// Hashes a string into a uint
// Credit to: http://www.cse.yorku.ca/~oz/hash.html - "djb2" algorithm
hashMap_uint hashMap_hashString(void* v){
	char* s = (char*) v;
    hashMap_uint h = 5381,c;
	while(c = *s++){
		h = ((h << 5) + h) + c; // h * 33 + c
	}
    return h;
}

// Chooses an appropriate size given the number of hashMap items
hashMap_uint hashMap_chooseSize(hashMap_uint n){
	hashMap_uint ret;
	short len = sizeof(HASHMAP_SIZES)/sizeof(hashMap_uint), i;
	for(i=0; i<len; ++i){
		ret = HASHMAP_SIZES[i];
		if(ret > n*2) break;
	}
	return ret;
}

// Create a map struct with a generic hashing function pointer
// *** N.B. Hash should never return 0 as that denotes error ***
hashMap* hashMap_create(hash_func f, hashMap_uint n){
	hashMap* ret = (hashMap*) malloc(sizeof(hashMap));
	ret->size = hashMap_chooseSize(n);
	ret->count = 0;
	ret->items = calloc(ret->size, sizeof(hashMapItem*));
	ret->hash = f;
	return ret;
}

// Change the capacity of a hashMap
void hashMap_resize(hashMap** h){
	// Create a new hashMap
	hashMap* oldMap = *h;
	hashMap* newMap = hashMap_create(oldMap->hash, oldMap->count);

	// Copy the items from h to the new hashMap
	if(oldMap->count){
		hashMap_uint k;
		for(k=0; k<oldMap->size; ++k){
			if(oldMap->items[k]){
				hashMap_uint insert = oldMap->items[k]->key % newMap->size;
				hashMap_uint i, j;
				for(i=0; i<newMap->size; ++i){
					j = (insert + i*i) % newMap->size;
					if(newMap->items[j]) continue;
					newMap->items[j] = oldMap->items[k];
					++newMap->count;
				}
			}
		}
	}

	// Free the old hashMap and have the pointer point to the new one
	free(oldMap->items);
	free(oldMap);
	*h = newMap;
}

// Frees a map struct with a generic destructor function pointer
void hashMap_destroy(hashMap* h, destruct_func f){
	// Return if hashmap has not been created
	if(!h) return;

	// Iterate through the items array and free any that exist
	if(h->count){
		hashMap_uint i;
		for(i=0; i<h->size; ++i){
			if(h->items[i]){
				// Call the supplied destructor function on each value
				f(h->items[i]->value);
				// Free the hashMap item wrapper
				free(h->items[i]);
				if(--h->count == 0) break;
			}
		}
	}

	// Free the item array
	free(h->items);
	// Free the hashMap struct itself
	free(h);
}

// Adds an item to a hashMap. Returns its key on successs else 0
hashMap_uint hashMap_add(hashMap* h, void* v){
	if(h->count*2 > h->size){
		hashMap_resize(&h);
	}

	// Create a new hashMap item to be inserted into the map
	hashMapItem* item = malloc(sizeof(hashMapItem));
	item->key = h->hash(v);
	item->value = v;

	// Find the correct index and insert (quadratic resolution)
	hashMap_uint insert = item->key % h->size, i, j;
	for(i=0; i<h->size; ++i){
		j = (insert + i*i) % h->size;
		if(h->items[j]){
			// Return 0 if an item with the same key exists
			if(h->items[j]->key == item->key) return 0;
			continue;
		}

		// Return the key once we insert successfully
		h->items[j] = item;
		++h->count;
		return item->key;
	}
}

// Retrieves an item from a hashMap by its key
void* hashMap_retrieve(hashMap* h, hashMap_uint k){
	// Find the correct index and fetch (quadratic resolution)
	hashMap_uint insert = k % h->size, i, j;
	for(i=0; i<h->size; ++i){
		j = (insert + i*i) % h->size;
		if(h->items[j]){
			// If an item with the same key exists, return it
			if(h->items[j]->key == k){
				return h->items[j]->value;
			}
		}

		// Return null if the item was not found
		return NULL;
	}
}

// Retrieves an item from a hashMap matching a new item
void* hashMap_search(hashMap* h, void* v){
	return hashMap_retrieve(h, h->hash(v));
}

// Remove one element from the hashMap by its key
void* hashMap_remove(hashMap* h, hashMap_uint k){
	// Find the correct index and remove (quadratic resolution)
	hashMap_uint insert = k % h->size, i, j;
	for(i=0; i<h->size; ++i){
		j = (insert + i*i) % h->size;
		if(h->items[j]){
			// If an item with the same key exists, remove it
			if(h->items[j]->key == k){
				void* ret = h->items[j]->value;
				free(h->items[j]);
				h->items[j] = 0;
				--h->count;
				return ret;
			}
		}

		// Return null if the item was not found
		return NULL;
	}
}
#endif
