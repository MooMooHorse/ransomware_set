import matplotlib.pyplot as plt
import numpy as np

import os
import shutil
import sys

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import PATHS
from config import cfs_type, FS_EXT2, FS_EXT4, FS_F2FS, FS_XFS, FS_NTFS
from config import print_red
from config import LOG_NAME

# Add the parent directory to sys.path
sys.path.append(parent_dir)

def plot_trace(trace_path):
    """
    For each line in the parsed file, the format is as follows:
    time(ns) sector_number bytes operation_type device_number(major, minor)
    operation_type : RWBS field, a (small) string (1-3 characters) 
    
    This is a small string containing at least one character ('R' for
       read, 'W' for write, or 'D' for block discard operation), and
       optionally either a 'B' (for barrier operations) or 'S' (for
       synchronous operations).
    """
    # read the trace file
    with open(trace_path, 'r') as f:
        lines = f.readlines()
        # parse the trace file
        # time, sector_number, bytes, operation_type, device_number
        for line in lines:
            line = line.strip()
            if not line:
                continue
            line = line.split(' ')
            line = [x for x in line if x != '']
            if (len(line) != 5) or ('R' not in line[3] and 'W' not in line[3]):
                continue
            timeNs = int(line[0])
            timeUs = timeNs / 1000
            sector_number = int(line[1])
            bytes = int(line[2])
            operation_type = line[3]
            device_number = int(line[4])
            

def check_log_legal(log_id):
    log_path = os.path.join(PATHS['log_dir'], 'logs_' + str(log_id))
    if not os.path.isdir(log_path):
        return False
    test_info_path = os.path.join(log_path, LOG_NAME['test_info'])
    if not os.path.isfile(test_info_path):
        return False
    mode = None
    timeout = None
    blknum = None
    threads = None
    access = None
    fsync = None
    rwsplit = None
    # read test_info_path
    with open(test_info_path, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith('mode='):
                mode = line.split('=')[1].rstrip('\n')
            elif line.startswith('timeout='):
                timeout = line.split('=')[1].rstrip('\n')
            elif line.startswith('blknum='):
                blknum = line.split('=')[1].rstrip('\n')
            elif line.startswith('threads='):
                threads = line.split('=')[1].rstrip('\n')
            elif line.startswith('access='):
                access = line.split('=')[1].rstrip('\n')
            elif line.startswith('fsync='):
                fsync = line.split('=')[1].rstrip('\n')
            elif line.startswith('rwsplit='):
                rwsplit = line.split('=')[1].rstrip('\n')

    if (mode == 'None' or timeout == 'None' or blknum == 'None' 
        or threads == 'None' or access == 'None' 
        or fsync == 'None' or rwsplit == 'None'):
        return False
    
    if (mode is None or timeout is None or blknum is None
            or threads is None or access is None
            or fsync is None or rwsplit is None):
        return False
    return True
def gather_data(dest_path, data_path = PATHS['log_dir']):
    """
    We first copy the data path to dest path.
    """
    if cfs_type == FS_EXT4:
        dest_path = dest_path + '_ext4'
    elif cfs_type == FS_EXT2:
        dest_path = dest_path + '_ext2'
    elif cfs_type == FS_F2FS:
        dest_path = dest_path + '_f2fs'
    elif cfs_type == FS_XFS:
        dest_path = dest_path + '_xfs'
    else:
        print_red("Unknown fs type to draw")
    
    if os.path.isdir(dest_path):
        shutil.rmtree(dest_path)
    os.mkdir(dest_path)
    for dirs in os.listdir(data_path):
        if not os.path.isdir(os.path.join(data_path, dirs)):
            continue
        log_id = int(dirs.split('_')[1].rstrip('\n'))
        if not check_log_legal(log_id):
            continue
        shutil.copytree(os.path.join(data_path, dirs), 
                        os.path.join(dest_path, dirs))
    shutil.copytree(os.path.join(PATHS['impression_path'], 'logs'), os.path.join(dest_path, 'impression_log'))
    shutil.copytree(os.path.join(PATHS['impression_path'], 'stat'), os.path.join(dest_path, 'impression_stat'))


def put_into_csv(paths):
    data_path, csv_dir = paths.split(',')
    suffix = ''
    if cfs_type == FS_EXT4:
        suffix = '_ext4'
    elif cfs_type == FS_EXT2:
        suffix = '_ext2'
    elif cfs_type == FS_F2FS:
        suffix = '_f2fs'
    elif cfs_type == FS_XFS:
        suffix = '_xfs'
    else:
        print_red("Unknown fs type to draw")
    data_path = data_path + suffix
    if not os.path.isdir(data_path):
        print_red("Data path does not exist")
        return
    if not os.path.isdir(csv_dir):
        os.mkdir(csv_dir)
    shutil.rmtree(csv_dir)
    os.mkdir(csv_dir)
    # create src{suffix}.csv
    src_csv_path = os.path.join(csv_dir, 'src' + suffix + '.csv')
    with open(src_csv_path, 'w') as f:
        # add header
        # mode, rtimeout, wtimeout, rblknum, wblknum, rthreads, wthreads, raccess, waccess, fsync, rwsplit, CBR
        f.write('mode,rtimeout,wtimeout,rblknum,wblknum,rthreads,wthreads,raccess,waccess,fsync,rwsplit,CBR\n')
    with open(src_csv_path, 'a') as f1:
        for dirs in os.listdir(data_path):
            if not os.path.isdir(os.path.join(data_path, dirs)):
                continue
            if not dirs.startswith('logs_'):
                continue
            log_path = os.path.join(data_path, dirs)
            test_info_path = os.path.join(log_path, LOG_NAME['test_info'])
            result_info_path = os.path.join(log_path, LOG_NAME['out'])
            # read mode, ...
            mode = None
            timeout = None
            blknum = None
            threads = None
            access = None
            fsync = None
            rwsplit = None
            with open(test_info_path, 'r') as f:
                lines = f.readlines()
                for line in lines:
                    if line.startswith('mode='):
                        mode = line.split('=')[1].rstrip('\n')
                    elif line.startswith('timeout='):
                        timeout = line.split('=')[1].rstrip('\n')
                    elif line.startswith('blknum='):
                        blknum = line.split('=')[1].rstrip('\n')
                    elif line.startswith('threads='):
                        threads = line.split('=')[1].rstrip('\n')
                    elif line.startswith('access='):
                        access = line.split('=')[1].rstrip('\n')
                    elif line.startswith('fsync='):
                        fsync = line.split('=')[1].rstrip('\n')
                    elif line.startswith('rwsplit='):
                        rwsplit = line.split('=')[1].rstrip('\n')
            if (mode == 'None' or timeout == 'None' or blknum == 'None'
                or threads == 'None' or access == 'None'
                or fsync == 'None' or rwsplit == 'None'):
                continue
            if (mode is None or timeout is None or blknum is None
                or threads is None or access is None
                or fsync is None or rwsplit is None):
                continue
            # read CBR = final / original
            CBR = None
            with open(result_info_path, 'r') as f:
                lines = f.readlines()
                for line in lines:
                    if line.startswith('original='):
                        original = int(line.split('=')[1].rstrip('\n'))
                    elif line.startswith('final='):
                        final = int(line.split('=')[1].rstrip('\n'))
            if (original == 'None' or final == 'None'):
                continue
            if (original is None or final is None):
                continue
            if original == 0:
                print_red("original is 0") # this should never happen
                continue
            CBR = str(final / original)
            rtimeout = timeout.split('/')[0].rstrip('\n')
            wtimeout = timeout.split('/')[1].rstrip('\n')
            rblknum = blknum.split('/')[0].rstrip('\n')
            wblknum = blknum.split('/')[1].rstrip('\n')
            rthreads = threads.split('/')[0].rstrip('\n')
            wthreads = threads.split('/')[1].rstrip('\n')
            raccess = access.split('/')[0].rstrip('\n')
            waccess = access.split('/')[1].rstrip('\n')
            fsync = str(int(fsync == 'Y'))
            rwsplit = str(int(rwsplit == 'Y'))
            # write to csv
            f1.write(f"{mode},{rtimeout},{wtimeout},{rblknum},{wblknum},{rthreads},{wthreads},{raccess},{waccess},{fsync},{rwsplit},{CBR}\n")

def handle_flags():
    """
    format of the flag: -gd=<dest_path> -csv=<data_path>,<csv_dir>
    """
    for arg in sys.argv:
        if arg.startswith('-gd='): 
            gather_data(arg.split('=')[1].rstrip('\n'))
        elif arg.startswith('-csv='):
            put_into_csv(arg.split('=')[1].rstrip('\n'))
    
if __name__ == '__main__':
    handle_flags()