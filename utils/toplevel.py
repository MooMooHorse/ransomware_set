import os
import shutil
import sys
import time


test_id = 1

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
from config import FS_EXT4, FS_NTFS, FS_F2FS, FS_EXT2, BOOT_CONFIG
from preprocess import preprocess_tar_sys


dev_list = MOUNT_CONFIG['dev_list']
dev_used = []

cfs_type = MOUNT_CONFIG['cfs_type']

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
        os.system(f"y | sudo mkfs.ntfs -f {dev_path} && sudo mount -t ntfs {dev_path} {mount_path}")
    elif cfs_type == FS_F2FS:
        os.system(f"y | sudo mkfs.f2fs -f {dev_path} && sudo mount -t f2fs {dev_path} {mount_path}")
    elif cfs_type == FS_EXT2:
        os.system(f"y | sudo mkfs.ext2 {dev_path} && sudo mount -t ext2 {dev_path} {mount_path}")
        

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
    
def prepare_tar_sys(tar_sys_path, _totsize, _mu, _fragscore, batch_ind):
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
    
    injectedRate = ['1%', '20%', '100%']
    injectedLimit = '100 MB' # must be in MB
    
    mu = degrade_mu(tot_size_unit, tot_size, mu)
    
    
    mount_dev(BOOT_CONFIG['default_disk'], tar_sys_path, cfs_type)

    # change the owner of the root_path to the current user
    os.system(f"sudo chown -R {os.getlogin()} {tar_sys_path}")
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
    
    for injected_rate in injectedRate:
        injected_size, inject_size_unit = rate2size(injected_rate, tot_size, tot_size_unit, injectedLimit) # injected size in MB
        log_dir = os.path.join(PATHS["log_dir"], f"logs_{test_id + BATCH_BASE}")
        if not os.path.isdir(log_dir):
            os.mkdir(log_dir)
        # remove all logs
        os.system(f"sudo rm -rf {log_dir}/*")
        # create log_dir/test_info
        with open(os.path.join(log_dir, test_info_path), 'w') as f:
            f.write(f"test_id : {test_id + BATCH_BASE}\n")
            f.write(f"gen_cmd : python3 {tar_sys_gen_path} -path={injected_path} -batch={test_id + BATCH_BASE} -tused={injected_size} -usedunit={inject_size_unit} -mu={mu} -fscore={fragscore}\n")
        # print(log_dir + '!!!!!!!!!!')
        
        mu = degrade_mu(inject_size_unit, injected_size, mu)
        # This will make sure that injected system is cleared before generating a new one
        os.system(f"python3 {tar_sys_gen_path} -path={injected_path} -batch={test_id + BATCH_BASE} -tused={injected_size} -usedunit={inject_size_unit} -mu={mu} -fscore={fragscore}") 
        preprocess_tar_sys(injected_path, log_dir, 'garbage')
        
        test_id += 1
        
        
    
import subprocess


def handle_flags(tar_sys_path, backup_dir_path):
    global framework_dir
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            impression_gen_path = PATHS["impression_gen_path"]
            # clean_up_groups_users(f"{framework_dir}") # first remove all groups and users created by gen_tar_sys.py
            # os.system(f"sudo umount {tar_sys_path} && sudo umount {backup_dir_path} && sudo rm -rf rans_test")
            os.system(f"sudo umount {tar_sys_path} && sudo rm -rf {tar_sys_path}")
            os.system(f"sudo rm -rf {framework_dir}/*")
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

handle_flags(tar_sys_path, backup_dir_path)


totoalSysSize = ['100 MB']
mu = ['4', '9.28', '17'] # to be tuned
fragScore = ['0', '0.5', '1']

start = time.time()

batch_ind = 1

for _totsize in totoalSysSize:
    for _mu in mu:
        for _fragscore in fragScore: 
            if batch_ind > 8:
                prepare_tar_sys(tar_sys_path, _totsize, _mu, _fragscore, batch_ind)
            batch_ind += 1

print(f"finish preparing target system using {time.time() - start} seconds")



# # Display backup paths and confirm user input
# print(f"Do you want to continue? with tar_sys_path = {tar_sys_path}, rans_path = {rans_path}, backup_dir_path = {backup_dir_path} (y/n)")
# input_val = input()
# if input_val != "y":
#     sys.exit(0)

# blktrace_dir = BOOT_CONFIG["blktrace_dir"]
# backup_blktrace_dir = BOOT_CONFIG["backup_blktrace_dir"]
# preprocess_tar_sys(tar_sys_path, blktrace_dir, BOOT_CONFIG["default_disk"])
# # preprocess_backup_dir(backup_dir_path, backup_blktrace_dir, BOOT_CONFIG["backup_disk"])

# os.system("./core")

