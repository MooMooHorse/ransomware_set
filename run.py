import os
import shutil
import sys

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG

def handle_flags():
    """Handle command-line flags.
        @note : This function must be ran outside the ransomware_set directory.
    """
    # Parse command-line arguments
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            # check if ransomware_set directory exists
            if not os.path.isdir("rans_test_framework"):
                print("rans_test_framework does not exist")
                print("clean up failed")
                sys.exit(1)
            # change directory to utils
            os.chdir("rans_test_framework/utils")
            os.system("python3 toplevel.py --clean")
            # change directory back to parent directory
            os.chdir("../..")
            blktrace_dir = BOOT_CONFIG["blktrace_dir"]
            blktrace_dir = f"rans_test_framework/{blktrace_dir}"
            if os.path.isfile(f"{blktrace_dir}/blktrace_pid"):
                # shut down blktrace whose pid is in blktrace_dir/blktrace_pid
                os.system(f"sudo pkill blktrace")

            # remove rams_test directory with sudo and exit
            os.system("sudo rm -rf rans_test_framework")
            print("clean up finished")
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
        elif arg.startswith("--menu"):
            print("Usage: python3 run.py [--clean]")
            print("All settings are in ransomware_set/config.py")
            sys.exit(0)
        elif arg.startswith("--genMini"):
            os.chdir("ransomware_set/utils/cryptosoft")
            os.system("make mini")
            os.system("mv mini ../../")
            sys.exit(0)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)

# go to parent directory of the current file's directory
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

handle_flags()

root_path = "rans_test_framework"
# mount_dev(dev_list[0], root_path, cfs_type)
# if root path does not exist, create it
if not os.path.isdir(root_path):
    os.mkdir(root_path)

# if the root_path is not empty, remove all files in it
if os.listdir(root_path):
    os.system(f"sudo rm -rf {root_path}/*")

# copy utils and config.py to root_path
shutil.copytree("ransomware_set/utils", f"{root_path}/utils")
shutil.copy("ransomware_set/config.py", f"{root_path}/config.py")
shutil.copy("ransomware_set/core", f"{root_path}/core")
shutil.copy("ransomware_set/mini", f"{root_path}/mini")

# cd to utils directory
os.chdir(f"{root_path}/utils")

# show current role (whoami)
print("This time the system will be tested underr current role:")
os.system("whoami")

# run the toplevel.py script
os.system(f"python3 toplevel.py")

