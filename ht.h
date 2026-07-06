#ifndef HT_H
#define HT_H


#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HT_KEY_TYPE
static_assert(0, "KEY_TYPE not defined");
#endif

#ifndef HT_VAL_TYPE
static_assert(0, "VAL_TYPE not defined");
#endif

#ifndef HT_HASH
static_assert(0, "HASH not defined, expected function signature: uint64_t (*)(KEY_TYPE)");
#endif

// CMP should return 0 if the keys are equal
#ifndef HT_CMP
static_assert(0, "CMP not defined, expected function signature: int (*)(KEY_TYPE, KEY_TYPE)");
#endif

// population factor
#ifndef HT_POP
#define HT_POP 0.75
#endif

#undef HT_KEY_VALUE
#undef HT_HASH_TABLE
#undef HT_INIT
#undef HT_DEINIT
#undef HT_INSERT
#undef HT_GET_OR_INSERT
#undef HT_GET
#undef HT_DELETE
#undef __HT_REALLOC
#undef HT_FREE
#undef HT_OCCUPIED
#undef HT_DELETED
#ifdef HT_PREFIX
#define __concat(a, b) a ## b
#define __concat2(a, b) __concat(a, b)
#define __change_name(a) __concat2(HT_PREFIX, a)
#define HT_KEY_VALUE __change_name(_KV)
#define HT_HASH_TABLE __change_name(_HT)
#define HT_INIT __change_name(_ht_init)
#define HT_DEINIT __change_name(_ht_deinit)
#define HT_INSERT __change_name(_ht_insert)
#define HT_GET_OR_INSERT __change_name(_ht_get_or_insert)
#define HT_GET __change_name(_ht_get)
#define HT_DELETE __change_name(_ht_delete)
#define __HT_REALLOC __change_name(__ht_realloc)
#else
#define HT_KEY_VALUE KV
#define HT_HASH_TABLE HT
#define HT_INIT ht_init
#define HT_DEINIT ht_deinit
#define HT_INSERT ht_insert
#define HT_GET_OR_INSERT ht_get_or_insert
#define HT_GET ht_get
#define HT_DELETE ht_delete
#define __HT_REALLOC __ht_realloc
#endif

#define HT_FREE 0
#define HT_OCCUPIED 1
#define HT_DELETED 2

// -------------------------------------------------
// --------------- type declarations ---------------
// -------------------------------------------------

typedef struct {
  HT_KEY_TYPE key;
  HT_VAL_TYPE val;
  char occupied;
} HT_KEY_VALUE;

typedef struct {
  HT_KEY_VALUE* items;
  size_t count;
  size_t capacity;
} HT_HASH_TABLE;

// -------------------------------------------------
// ------------- function declarations -------------
// -------------------------------------------------

// creates a new hash table with initial size.
HT_HASH_TABLE HT_INIT(size_t inital_size);

// frees all memory associated with hash table
void HT_DEINIT(HT_HASH_TABLE* ht);

// insert key-value pair with key and val,
// if an item with the same key exists, its value will be overwritten.
//
// if the hash table is full, it will reallocate, this is quite expensive,
// to counteract this, just increase the initial size in ht_init,
// or define NO_RESIZE, the hash table will then just error.
void HT_INSERT(HT_HASH_TABLE* ht, HT_KEY_TYPE key, HT_VAL_TYPE val);

// get a reference to the key-value pair with the corresponding key,
// if it does not exist, a new item is inserted.
HT_KEY_VALUE* HT_GET_OR_INSERT(HT_HASH_TABLE* ht, HT_KEY_TYPE key);

// get a reference to the key-value pair with the corresponding key,
// if it does not exist, this function returns NULL.
HT_KEY_VALUE* HT_GET(HT_HASH_TABLE ht, HT_KEY_TYPE key);

// deletes key-value pair with corresponding key.
// try to avoid this function, since the find function will be slow,
// if the item isn't in the table
void HT_DELETE(HT_HASH_TABLE* ht, HT_KEY_TYPE key);

// -------------------------------------------------
// ------------ function implementation ------------
// -------------------------------------------------

#ifndef HT_NO_IMPLEMENTATION

void __HT_REALLOC(HT_HASH_TABLE *ht, size_t s) {
#ifdef NO_RESIZE
  fprintf(stderr, "ERROR: hashtable full\n");
  exit(1);
#else // NO_RESIZE
  HT_KEY_VALUE* old_items = ht->items;
  size_t old_capacity = ht->capacity;
  ht->items = calloc(s, sizeof(HT_KEY_VALUE));
  ht->capacity = s;
  for(size_t i = 0; i < old_capacity; i++) {
    if(old_items[i].occupied) HT_INSERT(ht, old_items[i].key, old_items[i].val);
  }

  free(old_items);
#endif // NO_RESIZE
}


HT_HASH_TABLE HT_INIT(size_t initial_size) {
  HT_HASH_TABLE ht = {0};
  ht.capacity = initial_size;
  ht.items = calloc(initial_size, sizeof(HT_KEY_VALUE));
  if(!ht.items) {
    fprintf(stderr, "ERROR: memory allocation failed: %s", strerror(errno));
  }

  return ht;
}

void HT_DEINIT(HT_HASH_TABLE* ht) {
  assert(ht->capacity && "hashtable isn't initialised");
  free(ht->items);
  ht->capacity = 0;
  ht->count = 0;
}

void HT_INSERT(HT_HASH_TABLE* ht, HT_KEY_TYPE key, HT_VAL_TYPE val) {
  assert(ht->capacity && "hashtable isn't initialised");
  if(ht->count >= ht->capacity * HT_POP) __HT_REALLOC(ht, ht->capacity * 2);

  uint64_t h = HT_HASH(key) % ht->capacity;

  for(size_t i = 0; i < ht->capacity; i++) {
    if(!ht->items[h].occupied || HT_CMP(ht->items[h].key, key) == 0) break;
    h = ((h + 1) == ht->capacity) ? 0 : (h + 1);
  }

  if(!ht->items[h].occupied) ht->items[h].key = key;
  ht->items[h].occupied = 1;
  ht->items[h].val = val;
  ht->count++;
}

HT_KEY_VALUE* HT_GET_OR_INSERT(HT_HASH_TABLE* ht, HT_KEY_TYPE key) {
  assert(ht->capacity && "hashtable isn't initialised");
  if(ht->count >= ht->capacity * HT_POP) __HT_REALLOC(ht, ht->capacity * 2);

  uint64_t h = HT_HASH(key) % ht->capacity;

  for(size_t i = 0; i < ht->capacity; i++) {
    if(!ht->items[h].occupied) {
      ht->items[h].occupied = HT_OCCUPIED;
      ht->items[h].key = key;
      ht->count++;
      return &ht->items[h];
    }
    if(HT_CMP(ht->items[h].key, key) == 0) return &ht->items[h];
    h = ((h + 1) == ht->capacity) ? 0 : (h + 1);
  }

  assert(0 && "unreachable");
}

HT_KEY_VALUE* HT_GET(HT_HASH_TABLE ht, HT_KEY_TYPE key) {
  assert(ht.capacity && "hashtable isn't initialised");
  uint64_t h = HT_HASH(key) % ht.capacity;

  for(size_t i = 0; i < ht.capacity; i++) {
    if(!ht.items[h].occupied) return NULL;
    if(ht.items[h].occupied == HT_OCCUPIED && HT_CMP(ht.items[h].key, key) == 0) return &ht.items[h];
    h = ((h + 1) == ht.capacity) ? 0 : (h + 1);
  }

  return NULL;
}

void HT_DELETE(HT_HASH_TABLE* ht, HT_KEY_TYPE key) {
  assert(ht->capacity && "hashtable isn't initialised");
  HT_KEY_VALUE* kv = HT_GET(*ht, key);
  kv->occupied = HT_DELETED;
  ht->count--;

  if(ht->count < (ht->capacity >> 1) * HT_POP) {
    __HT_REALLOC(ht, ht->capacity >> 1);
  }
}

#endif // HT_NO_IMPLEMENTATION

#endif // HT_H
