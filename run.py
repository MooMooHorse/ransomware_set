import os
import shutil
import sys


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

# go to parent directory of the current file's directory
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

mount_path = "rans_test_framework"
mount_dev(dev_list[0], mount_path, cfs_type)

# change the owner of the mount_path to the current user
os.system(f"sudo chown -R {os.getlogin()} {mount_path}")

# copy utils and config.py to mount_path
shutil.copytree("ransomware_set/utils", f"{mount_path}/utils")
shutil.copy("ransomware_set/config.py", f"{mount_path}/config.py")

# cd to utils directory
os.chdir(f"{mount_path}/utils")

# show current role (whoami)
print("This time the system will be tested underr current role:")
os.system("whoami")

# run the toplevel.py script
os.system(f"python3 toplevel.py")
