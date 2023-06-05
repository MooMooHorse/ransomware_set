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
    .ino = {
        [0 ... TAR_FILE_MAX - 1] = 0
    }
};

void parse_kiocb_flags(struct file* file) {
    // haor2[tmp] : checked, no flag added. 
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
        if(inode->i_ino == CONFIG.ino[i]) {
            return 1;
        }
    }
    return 0;
}

int DIAG_FILE_IS_TAR(struct file* file) {
	if(file == NULL) {
		return 0;
	}
	const char* name = file->f_path.dentry->d_name.name;
	int i;
	
	for(i = 0; i < TAR_FILE_MAX; i++) {
		if(strcmp(name, CONFIG.target_files[i]) == 0) {
			// printk(KERN_ERR "found it !\n");
			return 1;
		}
	}
	return 0;
}

// return 0 if file is in target_files
int dumpNcache_file(struct file* file, struct inode *inode) {
    if(file == NULL) {
		return 0;
	}
	const char* name = file->f_path.dentry->d_name.name;
	int i;
	
	for(i = 0; i < TAR_FILE_MAX; i++) {
		if(strcmp(name, CONFIG.target_files[i]) == 0) {
			CONFIG.ino[i] = inode->i_ino;
            printk(KERN_ERR "found #inode = %lu\n", inode->i_ino);
			return 1;
		}
	}
	return 0;
}

