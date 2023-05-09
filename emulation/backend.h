#ifndef __BACKEND_H__
#define __BACKEND_H__

int init_dram_space(void*phy_loc, void* virt_loc, unsigned int nbytes);

/* Memory Backend (mbe) for emulated SSD */

int init_dram_backend(struct SsdDramBackend **mbe, size_t nbytes, void* phy_loc);


void free_dram_backend(struct SsdDramBackend *b);

// read or write to dram location
int backend_rw(struct SsdDramBackend *b, unsigned long ppa, void* data, bool is_write);

// read or write to dram location
int cache_rw(struct SsdDramBackend *b, unsigned long off, void* data, bool is_write,unsigned long size);

void *cache_mapped(struct SsdDramBackend *b, unsigned long off);

#endif
