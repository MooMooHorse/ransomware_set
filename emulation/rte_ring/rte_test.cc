#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "rte_ring.h"
#include "../simple_nvme.h"


int main()
{

    enum femu_ring_type type = FEMU_RING_TYPE_SP_SC;
    struct rte_ring *r = femu_ring_create(FEMU_RING_TYPE_SP_SC, 128);
    int add = 123123;
    NvmeCmd *a = (NvmeCmd*)malloc(sizeof(NvmeCmd));
    NvmeCmd *b = (NvmeCmd*)malloc(sizeof(NvmeCmd));
    NvmeCmd *c = (NvmeCmd*)malloc(sizeof(NvmeCmd));
    init_nvme(a,NVME_CMD_READ,11,12,&add);
    init_nvme(b,NVME_CMD_WRITE,15,32,&add);
    init_nvme(c,NVME_CMD_READ,12,2,&add);

    femu_ring_enqueue(r, (void *)&a,1);
    femu_ring_enqueue(r, (void *)&b,1);
    femu_ring_enqueue(r, (void *)&c ,1);
    NvmeRwCmd *d = (NvmeRwCmd*)malloc(sizeof(NvmeRwCmd));

    size_t ret = femu_ring_dequeue(r, (void *)&d,1);
    printf("return: %d",ret);
    printf("opcode: %d",d->opcode);
    printf("nsid: %d",d->nsid);
    printf("slba: %d",d->slba);
    printf("nlb: %d",d->nlb);
    printf("prp1: %x",d->prp1);
    return 0;
}