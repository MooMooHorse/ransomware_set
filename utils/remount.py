import os
import shutil
import sys
import time

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from access import clean_up_groups_users
from config import PATHS, TAR_SYS_PARAMS, MOUNT_CONFIG
from config import FS_EXT4, FS_NTFS, FS_F2FS, FS_EXT2, BOOT_CONFIG
from preprocess import preprocess_tar_sys, preprocess_backup_dir


dev_list = MOUNT_CONFIG['dev_list']
dev_used = []

cfs_type = MOUNT_CONFIG['cfs_type']

def remount_dev(dev_path, mount_path, cfs_type):
    # check if mount_path is already mounted
    if os.path.ismount(mount_path):
        # unmount it
        os.system(f"sudo umount {mount_path}")
    # if mount_path does not exist, create it
    if not os.path.isdir(mount_path):
        os.mkdir(mount_path)
    if cfs_type == FS_EXT4:
        # mkfs.ext4
        os.system(f"sudo mount -t ext4 {dev_path} {mount_path}")
    elif cfs_type == FS_NTFS:
        os.system(f"sudo mount -t ntfs {dev_path} {mount_path}")
    elif cfs_type == FS_F2FS:
        os.system(f"sudo mount -t f2fs {dev_path} {mount_path}")
    elif cfs_type == FS_EXT2:
        os.system(f"sudo mount -t ext2 {dev_path} {mount_path}")
        

    dev_list.remove(dev_path)
    dev_used.append(dev_path)
    
    
def handle_flag():
    for arg in sys.argv[1:]:
        if arg.startswith("-rmount"):
            mount_path = arg.split("=", 1)[1]
    
if __name__ == '__main__':
    handle_flag()