import os
import shutil
import sys

# Get the parent directory path
parent_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Add the parent directory to sys.path
sys.path.append(parent_dir)

from config import BOOT_CONFIG

cur_path = os.path.dirname(os.path.abspath(__file__))
toplevel_path = os.path.join(cur_path, 'utils', 'toplevel.py')

def handle_flags():
    """Handle command-line flags.
        @note : This function must be ran outside the ransomware_set directory.
    """
    # Parse command-line arguments
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            os.system(f"python3 {toplevel_path} --clean") 
            print("clean up finished")
            sys.exit(0)
        else:
            print("Invalid flag:", arg)
            sys.exit(1)

# go to parent directory of the current file's directory
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

handle_flags()

# run the toplevel.py script
os.system(f"python3 {toplevel_path}")

