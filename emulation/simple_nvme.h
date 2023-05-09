#ifndef SIMPLE_NVME_H_
#define SIMPLE_NVME_H_

#include "rte_ring/rte_ring.h"
#include <stdint.h>


typedef struct NvmeSglDescriptor 
{
    uint64_t addr;
    uint32_t len;
    uint8_t  rsvd[3];
    uint8_t  type;
} NvmeSglDescriptor;



typedef union NvmeCmdDptr {
    struct {
        uint64_t    prp1;
        uint64_t    prp2;
    };

    NvmeSglDescriptor sgl;
} NvmeCmdDptr;


typedef struct NvmeCmd {
    uint16_t    opcode : 8;
    uint16_t    fuse   : 2;
    uint16_t    res1   : 4;
    uint16_t    psdt   : 2;
    uint16_t    cid;
    uint32_t    nsid;
    uint64_t    res2;
    uint64_t    mptr;
    NvmeCmdDptr dptr;
    uint32_t    cdw10;
    uint32_t    cdw11;
    uint32_t    cdw12;
    uint32_t    cdw13;
    uint32_t    cdw14;
    uint32_t    cdw15;
} NvmeCmd;



enum NvmeIoCommands {
    NVME_CMD_FLUSH              = 0x00,
    NVME_CMD_WRITE              = 0x01,
    NVME_CMD_READ               = 0x02,
    NVME_CMD_WRITE_UNCOR        = 0x04,
    NVME_CMD_COMPARE            = 0x05,
    NVME_CMD_WRITE_ZEROES       = 0x08,
    NVME_CMD_DSM                = 0x09,
    NVME_CMD_ZONE_MGMT_SEND     = 0x79,
    NVME_CMD_ZONE_MGMT_RECV     = 0x7a,
    NVME_CMD_ZONE_APPEND        = 0x7d,
    NVME_CMD_OC_ERASE           = 0x90,
    NVME_CMD_OC_WRITE           = 0x91,
    NVME_CMD_OC_READ            = 0x92,
};


typedef struct NvmeIdentity {
    uint8_t     opcode;
    uint8_t     flags;
    uint16_t    cid;
    uint32_t    nsid;
    uint64_t    rsvd2[2];
    uint64_t    prp1;
    uint64_t    prp2;
    uint32_t    cns;
    uint16_t    nvmsetid;
    uint8_t     rsvd11;
    uint8_t     csi;
    uint32_t    rsvd12[4];
} NvmeIdentify;

typedef struct NvmeRwCmd {
    uint8_t     opcode;
    uint8_t     flags;
    uint16_t    cid;
    uint32_t    nsid;
    uint64_t    rsvd2;
    uint64_t    mptr;
    uint64_t    prp1;
    uint64_t    prp2;
    uint64_t    slba;
    uint16_t    nlb;
    uint16_t    control;
    uint32_t    dsmgmt;
    uint32_t    reftag;
    uint16_t    apptag;
    uint16_t    appmask;
} NvmeRwCmd;




typedef struct cntrl_event {
    uint16_t        event_type;
    volatile int    completed;
    uint64_t        s_time;
    uint64_t        e_time; 
    uint64_t        reqlat;
    uint64_t        expire_time; 

    uint64_t        identification;
    
    NvmeCmd         cmd;   

} event;




#endif