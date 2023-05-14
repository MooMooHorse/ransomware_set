# input format
# <duplicate_dir> <tar_sys_path> <backup_dir_path>
# run format
# python mkdup.py  <duplicate_dir> <tar_sys_path> <backup_dir_path>
import os
import shutil
import sys

# Check if correct input format is provided
if len(sys.argv) != 4:
    print("Usage: python create_duplicate_dir.py <duplicate_dir> <tar_sys_path> <backup_dir_path>")
    sys.exit(1)

# Directory paths
duplicate_dir = sys.argv[1]
tar_sys_path = sys.argv[2]
backup_dir_path = sys.argv[3]

# Remove duplicate directory if it exists
if os.path.isdir(duplicate_dir):
    print("Removing duplicate directory...")
    shutil.rmtree(duplicate_dir)
    print("Duplicate directory removed")

print("Creating duplicate directory...")
# Create duplicate directory and subdirectories
os.makedirs(duplicate_dir)
# os.makedirs(os.path.join(duplicate_dir, "duplicate_tar_sys"))
# # if backup_dir_path exists, create duplicate_backup_dir
# if os.path.isdir(backup_dir_path):
#     os.makedirs(os.path.join(duplicate_dir, "duplicate_backup_dir"))

# Copy tar_sys_path and backup_dir_path to duplicate directory
shutil.copytree(tar_sys_path, os.path.join(duplicate_dir, "duplicate_tar_sys"))
if os.path.isdir(backup_dir_path):
    shutil.copytree(backup_dir_path, os.path.join(duplicate_dir, "duplicate_backup_dir"))

print("Duplicate directory created")
