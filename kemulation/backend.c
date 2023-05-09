#include <linux/time.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/printk.h>
// #include <stdbool.h>
#include <linux/gfp.h>


#include "ftl.h"

// we don't zero init the memory region
int init_dram_space(void*phy_loc, void* virt_loc, unsigned int nbytes){
    return 0;
}

/* Memory Backend (mbe) for emulated SSD */

int init_dram_backend(struct SsdDramBackend **mbe, size_t nbytes, phys_addr_t phy_loc)
{
    struct SsdDramBackend *b = *mbe = (struct SsdDramBackend*)kzalloc(sizeof(struct SsdDramBackend),GFP_KERNEL);

    b->size = nbytes;
    b->phy_loc = phy_loc;

    // if (mlock(b->logical_space, nbytes) == -1) {
        // femu_err("Failed to pin the memory backend to the host DRAM\n");
        // free(b->logical_space);
        // abort();
    // }
    //TODO init or check here?
    // void* virt_loc = request_mem_region_exclusive((phys_addr_t)phy_loc,nbytes,"bytefs_be");

    // use malloc for non kernel version
    // void* virt_loc = kmalloc(nbytes+CACHE_SIZE);
    // if(!virt_loc){
    //     printk(KERN_ERR "phy_loc_exclusive_failure\n");
    //     // goto failed;
    // }
    // printk(KERN_ERR "phy_loc %llu\n",phy_loc);
    // virt_loc = ioremap_cache(phy_loc,nbytes); //@TODO check this
    // init_dram_space(phy_loc,virt_loc,nbytes);

    void* virt_loc = vmalloc(ALL_TOTAL_SIZE);

    if(!virt_loc){
        printk(KERN_ERR "map failure\n");
        goto failed;
    }

    b->virt_loc = virt_loc;
    printk(KERN_ERR "vir_loc %llu\n",virt_loc);
    return 0;
failed:
    return -1;
}


void free_dram_backend(struct SsdDramBackend *b)
{
    // TODO no need for free?

    // if (b->logical_space) {
    //     munlock(b->logical_space, b->size);
    //     g_free(b->logical_space);
    // }

    // no need to use this in non-kernel version
    // iounmap(b->virt_loc);

    kfree(b->virt_loc);
    kfree(b);
    return;
}

// read or write to dram location
int backend_rw(struct SsdDramBackend *b, unsigned long ppa, void* data, bool is_write)
{
    void* mb = b->virt_loc;
    void* page_addr = mb + (ppa*PG_SIZE);
    int ret=0;

    if (is_write){
        memcpy(page_addr,data,PG_SIZE);
    }else{
        memcpy(data,page_addr,PG_SIZE);
    }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
    return 0;
}


// read or write to dram location
int cache_rw(struct SsdDramBackend *b, unsigned long off, void* data, bool is_write,unsigned long size)
{
    void* mb = b->virt_loc;
    void* page_addr = mb + TOTAL_SIZE+off;
    if (off+size > CACHE_SIZE ) return -1;
    int ret=0;

    if (is_write){
        memcpy(page_addr,data,size);
    }else{
        memcpy(data,page_addr,size);
    }
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
    return 0;
}

// return dram translated location
void *cache_mapped(struct SsdDramBackend *b, unsigned long off) {
    return b->virt_loc + TOTAL_SIZE + off;
}



// for test only
// int main( ){
//     // init dram backend
//     struct SsdDramBackend *b;
//     size_t bytes = TOTAL_SIZE;
//     init_dram_backend(&b, bytes, (void*)0x0000000000);
//     printf("init dram backend done\n");

//     // write page to dram
//     char page[1024*16] = "hello world";
//     backend_rw(b,0,page,1);
//     printf("write to dram done\n");

//     // read from dram
//     char* data2 = (char*)malloc(PG_SIZE);
//     backend_rw(b,0,data2,0);
//     printf("read from dram done\n");
//     printf("data2: %s\n",data2);

//     // free dram backend
//     free_dram_backend(b);
//     printf("free dram backend done\n");

//     return 0;
// }