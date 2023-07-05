import os
import shutil
import sys
import time



ENABLE_DEBUG = False

def debug(*args, **kwargs):
    if ENABLE_DEBUG:
        print(*args, **kwargs)
        

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

# from access import clean_up_groups_users
from config import PATHS, TAR_SYS_PARAMS, MOUNT_CONFIG, BATCH_BASE, LOG_NAME
from config import FS_EXT4, FS_NTFS, FS_F2FS, FS_EXT2, FS_XFS, FS_BTRFS, FS_JBD, BOOT_CONFIG, config_file_path
from config import get_sys_config, get_test_id, get_batch_ind, get_rans_config, MODE_SEQ, MODE_RAND
from config import MODE_FROM_SCRATCH, MODE_CONTINUE, dispatch_rans_config
from preprocess import preprocess_tar_sys


test_id = get_test_id()
dev_list = MOUNT_CONFIG['dev_list']
dev_used = []

cfs_type = MOUNT_CONFIG['cfs_type']

start_record_bin = PATHS["start_record_bin"]
end_record_bin = PATHS["end_record_bin"]
blk2file_bin = PATHS["blk2file_bin"]
clear_record_bin = PATHS["clear_bin"]
core_path = PATHS["core_path"]

def mount_dev(dev_path, mount_path, cfs_type):
    # check if mount_path is already mounted
    if os.path.ismount(mount_path):
        # unmount it
        os.system(f"sudo umount {mount_path}")
    # if mount_path does not exist, create it
    if not os.path.isdir(mount_path):
        os.mkdir(mount_path)
    if cfs_type == FS_EXT4:
        # mkfs.ext4
        os.system(f"y | sudo mkfs.ext4 {dev_path} && sudo mount -t ext4 {dev_path} {mount_path}")
    elif cfs_type == FS_NTFS:
        os.system(f"sudo mkfs.ntfs -f {dev_path} && sudo mount -t ntfs {dev_path} {mount_path}")
    elif cfs_type == FS_F2FS:
        os.system(f"sudo mkfs.f2fs -f {dev_path} && sudo mount -t f2fs {dev_path} {mount_path}")
    elif cfs_type == FS_EXT2:
        os.system(f"sudo mkfs.ext2 -f {dev_path} && sudo mount -t ext2 {dev_path} {mount_path}")
    elif cfs_type == FS_XFS:
        os.system(f"sudo mkfs.xfs -f {dev_path} && sudo mount -t xfs {dev_path} {mount_path}")
    elif cfs_type == FS_BTRFS:
        os.system(f"sudo mkfs.btrfs -f {dev_path} && sudo mount -t btrfs {dev_path} {mount_path}")
    elif cfs_type == FS_JBD:
        os.system(f"y | sudo mkfs.ext4 {dev_path} -J size=512 && sudo mount -o data=journal -t ext4 {dev_path} {mount_path} && sudo LANG=C dumpe2fs {dev_path} | grep ^Journal")
       

    # dev_list.remove(dev_path)
    # dev_used.append(dev_path)
    
def degrade_mu(unit, size, mu):
    if unit == 'MB':
        if size <= 20:
            mu = min(mu, 8)
        elif size <= 150:
            mu = min(mu, 12)
        elif size <= 1000:
            mu = min(mu, 14)
    elif unit == 'GB':
        if size <= 1 :
            mu = min(mu, 15)
        elif size <= 2:
            mu = min(mu, 16)
            
    return mu
    
def prepare_tar_sys(tar_sys_path, _totsize, _mu, _fragscore, batch_ind, injected_rate):
    """re-construct the target system. Erase the original target system and create a new one.

    Args:
        tar_sys_path (string): path to the target system
        totsize (string): total size of the target system (approx.) in form of "xxx xB"
        mu (string): average peak of size distribution
        fragscore (string): fragmentation score
    """
    global test_id
    
    tar_sys_gen_path = PATHS["impression_gen_path"]
    injected_path = PATHS["injected_path"]
    test_info_path = LOG_NAME['test_info']
    tot_size = int(_totsize.split(' ')[0])
    tot_size_unit = _totsize.split(' ')[1]
    
    mu = float(_mu)
    fragscore = float(_fragscore)
    
    # injectedRate = ['1%', '20%', '100%']
    injectedLimit = '200 MB' # must be in MB
    
    mu = degrade_mu(tot_size_unit, tot_size, mu)
    
    # remove all files in the target system
    os.system(f"sudo rm -rf {tar_sys_path}/*") # ALARM, high overhead, be sure to do this only several times (in current case 27 times, only 9 of which are done to 100GB system)
    
    os.system(f"python3 {tar_sys_gen_path} -path={tar_sys_path} -batch={batch_ind} -tused={tot_size} -usedunit={tot_size_unit} -mu={mu} -fscore={fragscore}")
    
    def rate2size(_rate, tot_size, tot_size_unit, limit):
        if tot_size_unit == 'GB':
            tot_size = tot_size * 1024
            tot_size_unit = 'MB'
        rate = float(_rate.split('%')[0])
        if rate * tot_size < 100:
            return None, None
        limit_size = int(limit.split(' ')[0])
        if limit.split(' ')[1] == 'GB':
            limit_size = limit_size * 1024
        return min(rate * tot_size / 100, limit_size), 'MB'
    
    injected_size, inject_size_unit = rate2size(injected_rate, tot_size, tot_size_unit, injectedLimit) # injected size in MB

    dispatch_rans_config(MODE_FROM_SCRATCH)

    while True:
        _mode, _timeout, _blknum, _threads, _access, _fsync, _rwsplit = get_rans_config(MODE_RAND)
        if _mode is None or _timeout is None or _blknum is None or _threads is None or _access is None or _fsync is None or _rwsplit is None:
            break
        with open(PATHS["test_id_path"], 'w') as f:
            f.write(str(test_id + BATCH_BASE))

        with open(PATHS["test_dir_path_file"], 'w') as f:
            f.write(os.path.join(PATHS["log_dir"], f"logs_{test_id + BATCH_BASE}"))
        
        log_dir = os.path.join(PATHS["log_dir"], f"logs_{test_id + BATCH_BASE}")
        if not os.path.isdir(log_dir):
            os.mkdir(log_dir)
        # remove all logs
        os.system(f"sudo rm -rf {log_dir}/*")
        # create log_dir/test_info
        with open(os.path.join(log_dir, test_info_path), 'w') as f:
            f.write(f"test_id={test_id + BATCH_BASE}\n")
            f.write(f"batch_id={batch_ind}\n")
            f.write(f"tot_size={_totsize}\n")
            f.write(f"mu={_mu}\n")
            f.write(f"fragscore={_fragscore}\n")
            f.write(f"injected_rate={injected_rate}\n")
            f.write(f"injected_size={injected_size} {inject_size_unit}\n")
            f.write(f"mode={_mode}\n")
            f.write(f"timeout={_timeout}\n")
            f.write(f"blknum={_blknum}\n")
            f.write(f"threads={_threads}\n")
            f.write(f"access={_access}\n")
            f.write(f"fsync={_fsync}\n")
            f.write(f"rwsplit={_rwsplit}\n")
        
        mu = degrade_mu(inject_size_unit, injected_size, mu)
        # This will make sure that injected system is cleared before generating a new one
        os.system(f"python3 {tar_sys_gen_path} -path={injected_path} -batch={test_id + BATCH_BASE} -tused={injected_size} -usedunit={inject_size_unit} -mu={mu} -fscore={fragscore}") 

        os.system(start_record_bin) # start recording for blk2file mapping
        
        preprocess_tar_sys(injected_path, log_dir, 'garbage')

        os.system(end_record_bin)
        # print(core_path)
        os.system("sudo sync && echo 3 | sudo tee /proc/sys/vm/drop_caches")
        
        os.system(core_path + f" -path={config_file_path}" + f" -id={test_id + BATCH_BASE}")

        os.system(clear_record_bin)
        
        test_id += 1
        
def warmup(tar_sys_path):
    print("warmup start")
    tar_sys_gen_path = PATHS["impression_gen_path"]
    print(tar_sys_gen_path)
    os.system(f"python3 {tar_sys_gen_path} -path={tar_sys_path} -batch=1 -tused=10 -usedunit=GB -mu=9.48 -fscore=0.8")

    print("warmup allocated, need removal")

    # remove all files in the target system
    os.system(f"sudo rm -rf {tar_sys_path}/*") # ALARM, high overhead, be sure to do this only several times (in current case 27 times, only 9 of which are done to 100GB system)
    
    print("warmup finish")

import subprocess


def handle_flags(tar_sys_path, backup_dir_path):
    global framework_dir
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            config_dir = PATHS["config_dir"]
            config_dir_name = config_dir.split('/')[-1]
            impression_gen_path = PATHS["impression_gen_path"]
            # clean_up_groups_users(f"{framework_dir}") # first remove all groups and users created by gen_tar_sys.py
            # os.system(f"sudo umount {tar_sys_path} && sudo umount {backup_dir_path} && sudo rm -rf rans_test")
            os.system(f"sudo umount {tar_sys_path} && sudo rm -rf {tar_sys_path}")
            items = os.listdir(framework_dir)

            # Iterate through the items
            for item in items:
                item_path = os.path.join(framework_dir, item)
                
                # Skip the folder you want to keep
                if item == config_dir_name:
                    continue
                
                # Check if it's a file and remove it
                if os.path.isfile(item_path):
                    os.remove(item_path)
                
                # Check if it's a directory and remove it using shutil.rmtree
                elif os.path.isdir(item_path):
                    shutil.rmtree(item_path)
                    
            # print(f"sudo rm -rf {framework_dir}/!\({config_dir_name}\)")
            os.system(f"python3 {impression_gen_path} --clean")
            sys.exit(0)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)     

framework_dir = PATHS["framework_dir"]
tar_sys_path = PATHS["tar_sys_path"]
backup_dir_path = PATHS["backup_dir_path"]
log_dir_path = PATHS["log_dir"]

if not os.path.isdir(f"{framework_dir}"):
    os.mkdir(f"{framework_dir}")
if not os.path.isdir(log_dir_path):
    os.mkdir(log_dir_path)

if not os.path.isdir(PATHS["config_dir"]):
    os.mkdir(PATHS["config_dir"])

handle_flags(tar_sys_path, backup_dir_path)


batch_ind = get_batch_ind()

total_time = time.time()

mount_dev(BOOT_CONFIG['default_disk'], tar_sys_path, cfs_type)
# change the owner of the root_path to the current user
os.system(f"sudo chown -R {os.getlogin()} {tar_sys_path}")

# if you want to debug, comment warmup 
warmup(tar_sys_path)

while True:
    _totsize, _mu, _fragscore, injected_rate = get_sys_config(MODE_RAND)
    if _totsize == None or _mu == None or _fragscore == None or injected_rate == None:
        break
    start = time.time()
    prepare_tar_sys(tar_sys_path, _totsize, _mu, _fragscore, batch_ind, injected_rate)
    print(f"finish batch {batch_ind} using {time.time() - start} seconds")
    batch_ind += 1


print(f"finish all batches using {time.time() - total_time} seconds")

