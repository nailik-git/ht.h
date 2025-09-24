#ifndef HT_H
#define HT_H


#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef KEY_TYPE
static_assert(0, "KEY_TYPE not defined");
#endif

#ifndef VAL_TYPE
static_assert(0, "VAL_TYPE not defined");
#endif

#ifndef HASH
static_assert(0, "HASH not defined, expected function signature: uint64_t (*)(KEY_TYPE)");
#endif

// CMP should return 0 if the keys are equal
#ifndef CMP
static_assert(0, "CMP not defined, expected function signature: int (*)(KEY_TYPE, KEY_TYPE)");
#endif

// population factor
#ifndef POP
#define POP 0.75
#endif

#undef KEY_VALUE
#undef HASH_TABLE
#undef HT_INIT
#undef HT_DEINIT
#undef HT_INSERT
#undef HT_UPDATE
#undef HT_FIND
#undef HT_DELETE
#undef __HT_REALLOC
#ifdef PREFIX
#define __concat(a, b) a ## b
#define __concat2(a, b) __concat(a, b)
#define __change_name(a) __concat2(PREFIX, a)
#define KEY_VALUE __change_name(__concat2(_, KV))
#define HASH_TABLE __change_name(__concat2(_, HT))
#define HT_INIT __change_name(_ht_init)
#define HT_DEINIT __change_name(_ht_deinit)
#define HT_INSERT __change_name(_ht_insert)
#define HT_UPDATE __change_name(_ht_update)
#define HT_FIND __change_name(_ht_find)
#define HT_DELETE __change_name(_ht_delete)
#define __HT_REALLOC __change_name(__ht_realloc)
#else
#define KEY_VALUE KV
#define HASH_TABLE HT
#define HT_INIT ht_init
#define HT_DEINIT ht_deinit
#define HT_INSERT ht_insert
#define HT_UPDATE ht_update
#define HT_FIND ht_find
#define HT_DELETE ht_delete
#define __HT_REALLOC __ht_realloc
#endif

// -------------------------------------------------
// --------------- type declarations ---------------
// -------------------------------------------------

typedef struct {
  KEY_TYPE key;
  VAL_TYPE val;
  char occupied;
} KEY_VALUE;

typedef struct {
  KEY_VALUE* items;
  size_t count;
  size_t capacity;
} HASH_TABLE;

// -------------------------------------------------
// ------------- function declarations -------------
// -------------------------------------------------

// creates a new hash table with initial size.
HASH_TABLE HT_INIT(size_t inital_size);

// frees all memory associated with hash table
void HT_DEINIT(HASH_TABLE* ht);

// insert key-value pair with key and val,
// if an item with the same key exists, its value will be overwritten.
//
// if the hash table is full, it will reallocate, this is quite expensive,
// to counteract this, just increase the initial size in ht_init,
// or define NO_RESIZE, the hash table will then just error.
void HT_INSERT(HASH_TABLE* ht, KEY_TYPE key, VAL_TYPE val);

// get a reference to the key-value pair with the corresponding key,
// if it does not exist, a new item is inserted.
KEY_VALUE* HT_UPDATE(HASH_TABLE* ht, KEY_TYPE key);

// get a reference to the key-value pair with the corresponding key,
// if it does not exist, this function returns NULL.
KEY_VALUE* HT_FIND(HASH_TABLE ht, KEY_TYPE key);

// deletes key-value pair with corresponding key.
void HT_DELETE(HASH_TABLE* ht, KEY_TYPE key);

// -------------------------------------------------
// ------------ function implementation ------------
// -------------------------------------------------

#ifndef HT_NO_IMPLEMENTATION

void __HT_REALLOC(HASH_TABLE *ht, size_t s) {
#ifdef NO_RESIZE
  fprintf(stderr, "ERROR: hashtable full\n");
  exit(1);
#else // NO_RESIZE
  KEY_VALUE* old_items = ht->items;
  size_t old_capacity = ht->capacity;
  ht->items = calloc(s, sizeof(KEY_VALUE));
  ht->capacity = s;
  for(size_t i = 0; i < old_capacity; i++) {
    if(old_items[i].occupied) {
      uint64_t h = HASH(old_items[i].key) % ht->capacity;

      while(ht->items[h].occupied && CMP(ht->items[h].key, old_items[i].key) == 0) h++;

      ht->items[h].occupied = 1;
      ht->items[h].key = old_items[i].key;
      ht->items[h].val = old_items[i].val;
    }
  }

  free(old_items);
#endif // NO_RESIZE
}


HASH_TABLE HT_INIT(size_t initial_size) {
  HASH_TABLE ht = {0};
  ht.capacity = initial_size;
  ht.items = calloc(initial_size, sizeof(KEY_VALUE));
  if(!ht.items) {
    fprintf(stderr, "ERROR: memory allocation failed: %s", strerror(errno));
  }

  return ht;
}

void HT_DEINIT(HASH_TABLE* ht) {
  assert(ht->capacity && "hashtable isn't initialised");
  free(ht->items);
  ht->capacity = 0;
  ht->count = 0;
}

void HT_INSERT(HASH_TABLE* ht, KEY_TYPE key, VAL_TYPE val) {
  assert(ht->capacity && "hashtable isn't initialised");
  if(ht->count >= ht->capacity * POP) __HT_REALLOC(ht, ht->capacity * 2);

  uint64_t h = HASH(key) % ht->capacity;

  while(ht->items[h].occupied && CMP(ht->items[h].key, key) == 0) h = (h + 1) % ht->capacity;

  if(!ht->items[h].occupied) ht->items[h].key = key;
  ht->items[h].occupied = 1;
  ht->items[h].val = val;
  ht->count++;
}

KEY_VALUE* HT_UPDATE(HASH_TABLE* ht, KEY_TYPE key) {
  assert(ht->capacity && "hashtable isn't initialised");
  if(ht->count >= ht->capacity * POP) __HT_REALLOC(ht, ht->capacity * 2);

  uint64_t h = HASH(key) % ht->capacity;

  for(size_t i = 0; i < ht->capacity; i++) {
    if(!ht->items[h].occupied) {
      ht->items[h].occupied = 1;
      ht->items[h].key = key;
      ht->count++;
      return &ht->items[h];
    }
    if(CMP(ht->items[h].key, key) == 0) return &ht->items[h];
    h = (h + 1) % ht->capacity;
  }

  assert(0 && "unreachable");
}

KEY_VALUE* HT_FIND(HASH_TABLE ht, KEY_TYPE key) {
  assert(ht.capacity && "hashtable isn't initialised");
  uint64_t h = HASH(key) % ht.capacity;

  for(size_t i = 0; i < ht.capacity; i++) {
    if(!ht.items[h].occupied) break;
    if(CMP(ht.items[h].key, key) == 0) return &ht.items[h];
    h = (h + 1) % ht.capacity;
  }

  return NULL;
}

void HT_DELETE(HASH_TABLE* ht, KEY_TYPE key) {
  assert(ht->capacity && "hashtable isn't initialised");
  KEY_VALUE* kv = HT_FIND(*ht, key);
  kv->occupied = 0;
  ht->count--;

  if(ht->count < ht->capacity / 2 * POP) {
    __HT_REALLOC(ht, ht->capacity / 2);
  }
}

#endif // HT_NO_IMPLEMENTATION

#endif // HT_H
