import os
import shutil
import sys
import time
import json

framework_dir = "/home/h/rans_test"

cur_file_dir = os.path.dirname(os.path.realpath(__file__))
config_file_path = os.path.realpath(__file__)

impression_path = os.path.join(cur_file_dir, 'impressions')
impression_gen_path = os.path.join(os.path.join(impression_path, 'utils'), 'gen.py')
impression_draw_path = os.path.join(os.path.join(impression_path, 'utils'), 'draw.py')
impression_config_dir = os.path.join(impression_path, 'config')

core_path = os.path.join(cur_file_dir, 'core')

# a set of chosen file system types
FS_EXT4 = 0
FS_NTFS = 1
FS_F2FS = 2
FS_EXT2 = 3
FS_XFS = 4

cfs_type = FS_EXT4

NORMAL_DISK = 0
BACKUP_DISK = 1
LARGE_DISK = 2
XSMALL_DISK = 3

UTF8 = 0
UTF16 = 1

encoding_rule = UTF8

if cfs_type == FS_NTFS:
    encoding_rule = UTF16

MOUNT_CONFIG = {
    'dev_list' : ['/dev/vdb', '/dev/vdc', '/dev/vdd', '/dev/vde'],
    'cfs_type' : cfs_type,
    'enconding' : encoding_rule,
}

BOOT_CONFIG = {
    "default_disk" : MOUNT_CONFIG['dev_list'][NORMAL_DISK],
    "backup_disk" : MOUNT_CONFIG['dev_list'][BACKUP_DISK],
}


tar_sys_info_path = os.path.join(framework_dir, "tar_sys_info")
tar_sys_path = os.path.join(framework_dir, "tar_sys")
tar_sys_injected_path = os.path.join(tar_sys_path, "injected")
backup_dir = os.path.join(framework_dir, "backup_dir")
log_dir = os.path.join(framework_dir, "logs")
test_id_path = os.path.join(log_dir, 'test_id') # current test id is stored in this file
trace_path_file = os.path.join(log_dir, 'trace_path') # current trace path is stored in this file
blktrace_dir = os.path.join(framework_dir, "blktrace")
blktrace_result = os.path.join(log_dir, "blktrace")
syscall_dir = os.path.join(cur_file_dir, 'utils', 'syscall', 'bin')
start_record_bin = os.path.join(syscall_dir, 'start')
end_record_bin = os.path.join(syscall_dir, 'end')
blk2file_bin = os.path.join(syscall_dir, 'blk2file')
clear_bin = os.path.join(syscall_dir, 'clear')
rans_path = os.path.join(cur_file_dir, 'utils', 'preprocess.py')
rans_exec_path = os.path.join(cur_file_dir, 'utils', 'cryptosoft', 'ransomware.py')
test_dir_path_file = os.path.join(log_dir, 'test_dir_path') 
config_dir = os.path.join(framework_dir, 'rans_config')
rans_config_repos = os.path.join(config_dir, 'rans_repos')
rans_config_tested = os.path.join(config_dir, 'rans_tested')
sys_config_repos = os.path.join(config_dir, 'sys_repos')
sys_config_tested = os.path.join(config_dir, 'sys_tested')

BATCH_BASE = 100000 # a base to distinguish between config file for original system and injected system

# A set of paths used in the framework
PATHS = {
    "framework_dir": framework_dir,
    "tar_sys_path": tar_sys_path,
    "injected_path": tar_sys_injected_path,
    "backup_dir_path": backup_dir,
    "impression_path": impression_path,
    "impression_gen_path": impression_gen_path,
    "impression_draw_path": impression_draw_path,
    "log_dir" : log_dir,
    "test_id_path" : test_id_path,
    "blktrace_dir" : blktrace_dir,
    "blktrace_result" : blktrace_result,
    "syscall_dir" : syscall_dir,
    "start_record_bin" : start_record_bin,
    "end_record_bin" : end_record_bin,
    "blk2file_bin" : blk2file_bin,
    "clear_bin" : clear_bin,
    "rans_path" : rans_path,
    "core_path" : core_path,
    "rans_exec_path" : rans_exec_path,
    "trace_path_file" : trace_path_file,
    "test_dir_path_file" : test_dir_path_file,
    "config_dir" : config_dir,
    "rans_config_repos" : rans_config_repos,
    "rans_config_tested" : rans_config_tested,
    "sys_config_repos" : sys_config_repos,
    "sys_config_tested" : sys_config_tested,
}



LOG_NAME = {
    'test_info' : 'test_info.log',
    'dye_info' : 'dye_info.log',
    'trace_info' : 'trace.log',
    'parse_err' : 'parse_err.log',
    'rans_config' : 'rans_config.log',
    'sys_config' : 'sys_config.log',
    'out' : 'out.log',
}

# A set of parameters for the target system
TAR_SYS_PARAMS = {
    "number_of_files": 100,
    "median_length_of_files": 100000,
    "file_type_set": 1,
    # "num_groups" : 3,
    # "num_users" : 1,
    # "num_permissions" : 3,
    
}
# A set of magic numbers used in the framework
MAGIC_NUM = {
    # Magic number 1 is for the beginning of the file and directory name
    "MAGIC_NUM1_01" : 0x42,
    "MAGIC_NUM1_02" : 0x43,
    "MAGIC_NUM1_03" : 0x44,
    "MAGIC_NUM1_04" : 0x45,
    # Magic number 2 is for the end of the file and directory name
    "MAGIC_NUM2_01" : 0x22,
    "MAGIC_NUM2_02" : 0x23,
    "MAGIC_NUM2_03" : 0x24,
    "MAGIC_NUM2_04" : 0x25,
    # Magic number 3 is for each byte of the file data
    "MAGIC_NUM3" : 0x41,
    'DEBUG_MAGIC' : 'd',
}


# ANSI escape sequences for different colors
colors = {
    'red': '\033[91m',
    'green': '\033[92m',
    'blue': '\033[94m',
    'yellow': '\033[93m',
    'reset': '\033[0m'
}

def print_red(msg):
    print(colors['red'] + msg + colors['reset'])

def print_green(msg):
    print(colors['green'] + msg + colors['reset'])
    
def print_blue(msg):
    print(colors['blue'] + msg + colors['reset'])
    
def print_yellow(msg):
    print(colors['yellow'] + msg + colors['reset'])




MODE_FROM_SCRATCH = 0
MODE_CONTINUE = 1
def dispatch_rans_config(mode):
    if mode == MODE_FROM_SCRATCH:
        with open(rans_config_tested, 'w') as f:
            pass
        with open(rans_config_repos, 'w') as f:
            MODE = ['O', 'D', 'S'] # overwrite, delte, shred
            TIMEOUT = ['0/0', '0/10', '10/0', '10/10'] # 0 means no timeout 10 means 10 seconds
            BLKNUM = ['25000/25000', '25000/50000', '50000/25000'] # after this number of blocks, we trigger a timeout
            THREADS = ['1/1', '1/8', '8/1', '8/8'] # number of threads
            ACCESS  = ['R/R', 'R/S', 'S/R', 'S/S'] # access mode (random / sequential)
            for _mode in MODE:
                for _timeout in TIMEOUT:
                    for _blknum in BLKNUM:
                        for _threads in THREADS:
                            for _access in ACCESS:
                                f.write(f"{_mode} {_timeout} {_blknum} {_threads} {_access}\n")
    else:
        print("not supported yet")

def dispatch_sys_config(mode):
    if mode == MODE_FROM_SCRATCH:
        with open(sys_config_tested, 'w') as f:
            pass
        with open(sys_config_repos, 'w') as f:
            totoalSysSize = ['100 MB', '10 GB']
            mu = ['4', '9.28', '17'] # to be tuned
            fragScore = ['0', '0.5', '1']
            injectedRate = ['1%', '20%', '100%']
            for _totsize in totoalSysSize:
                for _mu in mu:
                    for _fragscore in fragScore:
                        for injected_rate in injectedRate:
                            f.write(f"{_totsize} {_mu} {_fragscore} {injected_rate}\n")
    elif mode == MODE_CONTINUE:
        tested = []
        with open(sys_config_tested, 'r') as f:
            lines = f.readlines()
            for line in lines:
                line = line.strip()
                if not line:
                    continue
                tested.append(line)
        with open(sys_config_repos, 'w') as f:
            totoalSysSize = ['100 MB', '10 GB']
            mu = ['4', '9.28', '17'] # to be tuned
            fragScore = ['0', '0.5', '1']
            injectedRate = ['1%', '20%', '100%']
            for _totsize in totoalSysSize:
                for _mu in mu:
                    for _fragscore in fragScore:
                        for injected_rate in injectedRate:
                            if f"{_totsize} {_mu} {_fragscore} {injected_rate}" not in tested:
                                f.write(f"{_totsize} {_mu} {_fragscore} {injected_rate}\n")

MODE_SEQ = 0
MODE_RAND = 1

def get_sys_config(mode):
    """
    Read from sys_config_repos to get a config, and put it into sys_config_tested.
    We need to remove the config from sys_config_repos after we get it.
    """
    if mode == MODE_SEQ:
        config = None
        with open(sys_config_repos, 'r') as f:
            lines = f.readlines()
            if not lines:
                return None, None, None, None
            config = lines[0]
        with open(sys_config_tested, 'a') as f:
            f.write(config)
        with open(sys_config_repos, 'w') as f:
            for line in lines[1:]:
                f.write(line)
        # break config into a list
        config = config.rstrip('\n')
        config = config.split(' ')
        return [config[0] + ' ' + config[1]] + config[2:]
    elif mode == MODE_RAND:
        config = None
        import random
        with open(sys_config_repos, 'r') as f:
            lines = f.readlines()
            if not lines:
                return None, None, None, None
            # randomly get a config
            config = lines[random.randint(0, len(lines) - 1)]
        with open(sys_config_tested, 'a') as f:
            f.write(config)
        with open(sys_config_repos, 'w') as f:
            for line in lines:
                if line != config:
                    f.write(line)
        # break config into a list
        config = config.rstrip('\n')
        config = config.split(' ')
        return [config[0] + ' ' + config[1]] + config[2:]

def get_rans_config(mode):
    """
    Read from rans_config_repos to get a config, and put it into rans_config_tested.
    We need to remove the config from rans_config_repos after we get it.
    """
    if mode == MODE_SEQ:
        config = None
        with open(rans_config_repos, 'r') as f:
            lines = f.readlines()
            if not lines:
                return None, None, None, None, None
            config = lines[0]
        with open(rans_config_tested, 'a') as f:
            f.write(config)
        with open(rans_config_repos, 'w') as f:
            for line in lines[1:]:
                f.write(line)
        # break config into a list
        config = config.rstrip('\n')
        config = config.split(' ')

        return config
    elif mode == MODE_RAND:
        config = None
        import random
        with open(rans_config_repos, 'r') as f:
            lines = f.readlines()
            if not lines:
                return None, None, None, None, None
            # randomly get a config
            config = lines[random.randint(0, len(lines) - 1)]
        with open(rans_config_tested, 'a') as f:
            f.write(config)
        with open(rans_config_repos, 'w') as f:
            for line in lines:
                if line != config:
                    f.write(line)
        # break config into a list
        config = config.rstrip('\n')
        config = config.split(' ')
        return config

def get_batch_ind():
    """
    Browse through impression_config_dir to get the largest batch index ( < BATCH BASE ).
    """
    batch_ind = 0
    if not os.path.isdir(impression_config_dir):
        return 1
    for config_file in os.listdir(impression_config_dir):
        if config_file.startswith('config_'):
            if int(config_file.split('_')[1]) < BATCH_BASE:
                batch_ind = max(batch_ind, int(config_file.split('_')[1]))
    return batch_ind + 1

def get_test_id():
    """
    Browse through log_dir to get the largest test id.
    The result is normalized by BATCH_BASE.
    """
    test_id = BATCH_BASE
    if not os.path.isdir(log_dir):
        return 1
    for log_file in os.listdir(log_dir):
        if log_file.startswith('logs_'):
            test_id = max(test_id, int(log_file.split('_')[1]))
    return test_id - BATCH_BASE + 1

def main():
    args = []
    log_dir = PATHS["log_dir"]
    default_disk = BOOT_CONFIG["default_disk"]
    backup_disk = BOOT_CONFIG["backup_disk"]
    trace_info_log_name = LOG_NAME["trace_info"]
    magic1 = ((MAGIC_NUM["MAGIC_NUM1_01"] << 24) + (MAGIC_NUM["MAGIC_NUM1_02"] << 16) + 
                (MAGIC_NUM["MAGIC_NUM1_03"] << 8) + (MAGIC_NUM["MAGIC_NUM1_04"]))
    magic2 = ((MAGIC_NUM["MAGIC_NUM2_01"] << 24) + (MAGIC_NUM["MAGIC_NUM2_02"] << 16) + 
        (MAGIC_NUM["MAGIC_NUM2_03"] << 8) + (MAGIC_NUM["MAGIC_NUM2_04"]))
    magic3 = MAGIC_NUM["MAGIC_NUM3"]
    for arg in sys.argv[1:]:
        if arg.startswith("-tfp"): # trace file path
            args.append(trace_path_file)
        elif arg.startswith("-dd"): # dump default_disk
            args.append(f"{default_disk}")
        elif arg.startswith("-log"): # dump log directory
            args.append(log_dir)
        elif arg.startswith("-ma"): # all magic numbers
            args.append(f"{magic1} {magic2} {magic3}")
        elif arg.startswith("-sys"): # dump system call related bins' path
            args.append(f"{start_record_bin} {end_record_bin} {blk2file_bin}")
        elif arg.startswith("-rans"): # ransomware path
            args.append(rans_path)
        elif arg.startswith("-tinfo"):
            args.append(trace_info_log_name)
        elif arg.startswith("-out"): # output log
            args.append(LOG_NAME["out"])
        elif arg.startswith("-abcdir"):
            args.append(test_dir_path_file)
        elif arg.startswith("-dissys=scratch"):
            dispatch_sys_config(MODE_FROM_SCRATCH)
        elif arg.startswith("-disrans=scratch"):
            dispatch_rans_config(MODE_FROM_SCRATCH)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)
    return args



if __name__ == "__main__":
    args = main()
    # dump args delimited by space
    print(" ".join(args))