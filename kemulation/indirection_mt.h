#ifndef __INDIRECTION_MT_H__
#define __INDIRECTION_MT_H__

// #include <stdint.h>

#define MTE_EXIST (1 << 0)
#define IN_LOG    (1 << 1)
#define RESERVED1 (1 << 2)
#define RESERVED2 (1 << 3)
#define RESERVED3 (1 << 4)
#define RESERVED4 (1 << 5)
#define RESERVED5 (1 << 6)
#define RESERVED6 (1 << 7)

void init_indirection_mte(struct ssd *ssd);
void imt_insert(struct ssd *ssd, unsigned long key, unsigned long value);
struct indirection_mte *imt_get(struct ssd *ssd, unsigned long key);
int imt_remove(struct ssd *ssd, unsigned long key);
int imt_set_flag(struct ssd *ssd, unsigned long key, unsigned char flag);
int imt_remove_flag(struct ssd *ssd, unsigned long key, unsigned char flag);
float imt_get_load_factor(struct ssd *ssd);

#endif 
