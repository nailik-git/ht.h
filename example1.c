#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

uint64_t hash(String_View s) {
  uint64_t h = 5381;
  
  for(size_t i = 0; i < s.count; i++) {
    h = ((h << 5) + h) ^ s.data[i];
  }

  return h;
}

int sv_eq_cmp(String_View key1, String_View key2) {
  if(key1.count != key2.count) return key1.count - key2.count;
  return memcmp(key1.data, key2.data, key1.count);
}

#define KEY_TYPE String_View
#define VAL_TYPE int
#define HASH(key) hash(key)
#define CMP(key1, key2) sv_eq_cmp(key1, key2)
#include "ht.h"

int compar(const void* a, const void* b) {
  const KV* kv1 = (KV*) a;
  const KV* kv2 = (KV*) b;

  return kv2->val - kv1->val;
}

String_View sv_chop_by_whitespace(Nob_String_View *sv)
{
    size_t i = 0;
    while (i < sv->count && !isspace(sv->data[i])) {
        i += 1;
    }

    String_View result = sv_from_parts(sv->data, i);

    if (i < sv->count) {
        sv->count -= i + 1;
        sv->data  += i + 1;
    } else {
        sv->count -= i;
        sv->data  += i;
    }

    return result;
}

int main() {
  String_Builder sb = {0};
  String_View sv = {0};
  String_View key = {0};

  HT ht = ht_init(1000);

  read_entire_file("test.txt", &sb);
  sv = sb_to_sv(sb);

  while(sv.count) {
    sv = sv_trim_left(sv);
    key = sv_chop_by_whitespace(&sv);

    KV* kv = ht_update(&ht, key);
    kv->val++;
  }

  qsort(ht.items, ht.capacity, sizeof(KV), compar);

  printf("Top 10 tokens:\n");
  for(int i = 0; i < 10; i++) {
    printf("  %.*s: %d\n", (int) ht.items[i].key.count, ht.items[i].key.data, ht.items[i].val);
  }

  sb_free(sb);
  ht_deinit(&ht);
  return 0;
}
