#include "diag.h"

#include <linux/fs.h>
#include <linux/path.h>

#include "../fs/ext4/ext4_jbd2.h"
#include "../fs/ext4/xattr.h"
#include "../fs/ext4/acl.h"
#include "../fs/ext4/truncate.h"

config_t CONFIG = {
    .target_files = {
        "debug_file"
#if TAR_FILE_MAX > 1
        ,
        [1 ... TAR_FILE_MAX - 1] = NULL
#endif
    },
    .inode_pter = {
        [0 ... TAR_FILE_MAX - 1] = NULL
    }
};

void parse_kiocb_flags(struct file* file) {
	int flag;
	flag = iocb_flags(file);
	if(flag & IOCB_APPEND) {
		printk(KERN_ERR "IOCB_APPEND\n");
	} 
	if(flag & IOCB_DIRECT) {
		printk(KERN_ERR "IOCB_DIRECT\n");
	}
	if(flag & IOCB_DSYNC) {
		printk(KERN_ERR "IOCB_DSYNC\n");
	}
	if(flag & IOCB_SYNC) {
		printk(KERN_ERR "IOCB_SYNC\n");
	}
}


/**
 * Check if inode is in target_files
*/
int DIAG_INODE_IS_TAR(struct inode* inode) {
    int i;
    if(inode == NULL) {
        return 0;
    }
    for(i = 0; i < TAR_FILE_MAX; i++) {
        if(inode == (struct inode*)CONFIG.inode_pter[i]) {
            return 1;
        }
    }
    return 0;
}

int DIAG_FILE_IS_TAR(struct file* file) {
	if(file == NULL) {
		return 0;
	}
	char* name;
	int i;
	name = file->f_path.dentry->d_name.name;
	
	for(i = 0; i < TAR_FILE_MAX; i++) {
		if(strcmp(name, CONFIG.target_files[i]) == 0) {
			printk(KERN_ERR "found it !\n");
			return 1;
		}
	}
	return 0;
}

// return 0 if file is in target_files
static int dumpNcache_file(struct file* file, struct inode *inode) {
    char* name;
    int i;
    int ret;

    ret = -1;
    name =file->f_path.dentry->d_name.name;

    for(i = 0; i < TAR_FILE_MAX; i++) {
        if(strcmp(name, CONFIG.target_files[i]) == 0) {
            printk(KERN_ERR "diag: %s with inode at %llx\n", name, (uint64_t)inode);
            CONFIG.inode_pter[i] = (uint64_t)inode;
            ret = 0;
        }
    }

    return ret;
}



#define IS_TARGET_FILE(file)    (dumpNcache_file(file) == 0)

int diag_block_write_begin(struct file *file, struct page *page, loff_t pos, unsigned len,
				  get_block_t *get_block)
{
	unsigned from = pos & (PAGE_SIZE - 1);
	unsigned to = from + len;
	struct inode *inode = page->mapping->host;
	unsigned block_start, block_end;
	sector_t block;
	int err = 0;
	unsigned blocksize = inode->i_sb->s_blocksize;
	unsigned bbits;
	struct buffer_head *bh, *head, *wait[2], **wait_bh = wait;
	bool decrypt = false;
	printk(KERN_ERR "----------wow--------------\n");

    dumpNcache_file(file, inode);

	BUG_ON(!PageLocked(page));
	BUG_ON(from > PAGE_SIZE);
	BUG_ON(to > PAGE_SIZE);
	BUG_ON(from > to);

	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);
	head = page_buffers(page);
	bbits = ilog2(blocksize);
	block = (sector_t)page->index << (PAGE_SHIFT - bbits);

	for (bh = head, block_start = 0; bh != head || !block_start;
	    block++, block_start = block_end, bh = bh->b_this_page) {
		block_end = block_start + blocksize;
		if (block_end <= from || block_start >= to) {
			if (PageUptodate(page)) {
				if (!buffer_uptodate(bh))
					set_buffer_uptodate(bh);
			}
			continue;
		}
		if (buffer_new(bh))
			clear_buffer_new(bh);
		if (!buffer_mapped(bh)) {
			WARN_ON(bh->b_size != blocksize);
			err = get_block(inode, block, bh, 1);
			if (err)
				break;
			if (buffer_new(bh)) {
				if (PageUptodate(page)) {
					clear_buffer_new(bh);
					set_buffer_uptodate(bh);
					mark_buffer_dirty(bh);
					continue;
				}
				if (block_end > to || block_start < from)
					zero_user_segments(page, to, block_end,
							   block_start, from);
				continue;
			}
		}
		if (PageUptodate(page)) {
			if (!buffer_uptodate(bh))
				set_buffer_uptodate(bh);
			continue;
		}
		if (!buffer_uptodate(bh) && !buffer_delay(bh) &&
		    !buffer_unwritten(bh) &&
		    (block_start < from || block_end > to)) {
			ll_rw_block(REQ_OP_READ, 0, 1, &bh);
			*wait_bh++ = bh;
			decrypt = IS_ENCRYPTED(inode) && S_ISREG(inode->i_mode);
		}
	}
	/*
	 * If we issued read requests, let them complete.
	 */
	while (wait_bh > wait) {
		wait_on_buffer(*--wait_bh);
		if (!buffer_uptodate(*wait_bh))
			err = -EIO;
	}
	if (unlikely(err))
		page_zero_new_buffers(page, from, to);
	else if (decrypt)
		err = fscrypt_decrypt_page(page->mapping->host, page,
				PAGE_SIZE, 0, page->index);
	return err;
}
EXPORT_SYMBOL(diag_block_write_begin);

