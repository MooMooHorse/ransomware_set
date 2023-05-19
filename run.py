import os
import shutil
import sys

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG


# a set of chosen file system types
FS_EXT4 = 0

dev_list = ['/dev/vdb', '/dev/vdc', '/dev/vdd']
dev_used = []

fs_types = ['ext4']
cfs_type = FS_EXT4 # chosen fs type

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
        os.system(f"sudo mkfs.ext4 {dev_path} && sudo mount -t ext4 {dev_path} {mount_path}")

    dev_list.remove(dev_path)
    dev_used.append(dev_path)

def handle_flags():
    # Parse command-line arguments
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            blktrace_dir = BOOT_CONFIG["blktrace_dir"]
            blktrace_dir = f"rans_test_framework/{blktrace_dir}"
            if os.path.isfile(f"{blktrace_dir}/blktrace_pid"):
                # shut down blktrace whose pid is in blktrace_dir/blktrace_pid
                os.system(f"sudo pkill blktrace")

            # remove rams_test directory with sudo and exit
            os.system("sudo umount rans_test_framework && sudo rm -rf rans_test_framework")
            sys.exit(0)
        elif arg.startswith("--closeblk"):
            blktrace_dir = BOOT_CONFIG["blktrace_dir"]
            blktrace_dir = f"rans_test_framework/{blktrace_dir}"
            # first check if blktrace_dir/blktrace_pid exists
            if not os.path.isfile(f"{blktrace_dir}/blktrace_pid"):
                print("blktrace_pid does not exist")
                sys.exit(1)
            # shut down blktrace whose pid is in blktrace_dir/blktrace_pid
            os.system(f"sudo pkill blktrace")
            sys.exit(0)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)

# go to parent directory of the current file's directory
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

handle_flags()

mount_path = "rans_test_framework"
mount_dev(dev_list[0], mount_path, cfs_type)

# change the owner of the mount_path to the current user
os.system(f"sudo chown -R {os.getlogin()} {mount_path}")

# copy utils and config.py to mount_path
shutil.copytree("ransomware_set/utils", f"{mount_path}/utils")
shutil.copy("ransomware_set/config.py", f"{mount_path}/config.py")
shutil.copy("ransomware_set/core", f"{mount_path}/core")

# cd to utils directory
os.chdir(f"{mount_path}/utils")

# show current role (whoami)
print("This time the system will be tested underr current role:")
os.system("whoami")

# run the toplevel.py script
os.system(f"python3 toplevel.py")

