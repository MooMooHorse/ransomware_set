#include <stdio.h>
#include <stdlib.h>

#include "ftl_mapping.h"
#include "backend.h"
#include "hashmap.h"

void init_indirection_mte(struct ssd *ssd) {
  ssd->indirection_mt = (struct indirection_mte *) cache_mapped(ssd->bd, INDIRECTION_MT_START);
  memset(ssd->indirection_mt, 0, INDIRECT_MT_ENTRIES * sizeof(struct indirection_mte));
  ssd->indirection_mt_size = 0;
}

// multiplicative hashing function
static unsigned long hash(unsigned long key) {
  // a large prime number
  const unsigned long a = 990590075063L;
  // word_size - log2(INDIRECT_MT_ENTRIES)
  const unsigned long b = 32 - 14;
  key ^= key >> b; 
  return ((a * key) >> b) % INDIRECT_MT_ENTRIES;
}

void imt_insert(struct ssd *ssd, unsigned long key, unsigned long value) {
  ftl_assert(ssd->indirection_mt_size < INDIRECT_MT_ENTRIES);
  unsigned long hkey = hash(key);
  unsigned long i = hkey;
  while (ssd->indirection_mt[i].flag & MTE_EXIST) {
    if (ssd->indirection_mt->lpa == key) {
      ssd->indirection_mt[i].addr = value;
      return;
    }
    i = (i + 1) % INDIRECT_MT_ENTRIES;
    ftl_assert(i != hkey);
  }
  ssd->indirection_mt[i].lpa = key;
  ssd->indirection_mt[i].addr = value;
  ssd->indirection_mt[i].flag |= MTE_EXIST;
  ssd->indirection_mt_size++;
}

struct indirection_mte *imt_get(struct ssd *ssd, unsigned long key) {
  unsigned long hkey = hash(key);
  unsigned long i = hkey;
  while (ssd->indirection_mt[i].flag & MTE_EXIST) {
    if (ssd->indirection_mt[i].lpa == key) {
      return &ssd->indirection_mt[i];
    }
    i = (i + 1) % INDIRECT_MT_ENTRIES;
    if (i == hkey)
      break;
  }
  return NULL;
}

int imt_remove(struct ssd *ssd, unsigned long key) {
  unsigned long hkey = hash(key);
  unsigned long i = hkey;
  while (ssd->indirection_mt[i].flag & MTE_EXIST) {
    if (ssd->indirection_mt[i].lpa == key) {
      ssd->indirection_mt[i].flag & (~MTE_EXIST);
      ssd->indirection_mt_size--;
      return 0;
    }
    i = (i + 1) % INDIRECT_MT_ENTRIES;
    if (i == hkey)
      break;
  }
  return -1;
}

int imt_set_flag(struct ssd *ssd, unsigned long key, unsigned char flag) {
  ftl_assert((flag & MTE_EXIST) == 0);
  struct indirection_mte *mte = imt_get(ssd, key);
  if (mte) {
    mte->flag |= flag;
    return 0;
  }
  return -1;
}

int imt_remove_flag(struct ssd *ssd, unsigned long key, unsigned char flag) {
  ftl_assert((flag & MTE_EXIST) == 0);
  struct indirection_mte *mte = imt_get(ssd, key);
  if (mte) {
    mte->flag &= ~flag;
    return 0;
  }
  return -1;
}

float imt_get_load_factor(struct ssd *ssd) {
  return (float) ssd->indirection_mt_size / INDIRECT_MT_ENTRIES;
}

