#ifndef __FTL_CACHE_H__
#define __FTL_CACHE_H__

#include "ftl.h"

// different entry sizes
#define INDIRECT_MT_ENTRIES         (26000)

#define LOG_SIZE_PG                 (500)
#define NUM_LOG_PER_PG              (PG_SIZE / sizeof(struct log_entry))
#define LOG_ENTRIES                 (LOG_SIZE_PG * PG_SIZE / sizeof(struct log_entry))

#define FLUSHED_ARRAY_ENTRIES       (800)

// indirection mapping table
#define LOG_START                   (0)
#define LOG_SIZE                    (LOG_SIZE_PG * PG_SIZE)
#define LOG_END                     (LOG_START + LOG_SIZE)

#define INDIRECTION_MT_START        (LOG_END)
#define INDIRECTION_MT_SIZE         (INDIRECT_MT_ENTRIES * sizeof(struct indirection_mte))
#define INDIRECTION_MT_END          (INDIRECTION_MT_START + INDIRECT_MT_ENTRIES)

#define FLUSHED_ARRAY_START         (INDIRECTION_MT_END)
#define FLUSHED_ARRAY_SIZE          (FLUSHED_ARRAY_ENTRIES * sizeof(unsigned long))
#define FLUSHED_ARRAY_END           (FLUSHED_ARRAY_START + FLUSHED_ARRAY_SIZE)

#define PG_WR_FLUSH_BUF_START       (FLUSHED_ARRAY_END)
#define PG_WR_FLUSH_BUF_SIZE        (PG_SIZE)
#define PG_WR_FLUSH_BUF_END         (PG_WR_FLUSH_BUF_START + PG_WR_FLUSH_BUF_SIZE)

#define PG_RD_FLUSH_BUF_START       (PG_WR_FLUSH_BUF_END)
#define PG_RD_FLUSH_BUF_SIZE        (PG_SIZE)
#define PG_RD_FLUSH_BUF_END         (PG_RD_FLUSH_BUF_START + PG_RD_FLUSH_BUF_SIZE)

#define PG_RD_NAND_BUF_START        (PG_RD_FLUSH_BUF_END)
#define PG_RD_NAND_BUF_SIZE         (PG_SIZE)
#define PG_RD_NAND_BUF_END          (PG_RD_NAND_BUF_START + PG_RD_NAND_BUF_SIZE)

#define MIGRATION_LOG_BUF_START     (PG_RD_NAND_BUF_END)
#define MIGRATION_LOG_BUF_SIZE      (PG_SIZE)
#define MIGRATION_LOG_BUF_END       (MIGRATION_LOG_BUF_START + MIGRATION_LOG_BUF_SIZE)

#define MIGRATION_DEST_BUF_START    (MIGRATION_LOG_BUF_END)
#define MIGRATION_DEST_BUF_SIZE     (PG_SIZE)
#define MIGRATION_DEST_BUF_END      (MIGRATION_DEST_BUF_START + MIGRATION_DEST_BUF_SIZE)

#define DRAM_CACHE_END              (MIGRATION_DEST_BUF_END)

#endif
