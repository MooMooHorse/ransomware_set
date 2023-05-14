import os
import sys
import shutil

# Check if correct input format is provided
if len(sys.argv) != 7 and len(sys.argv) != 4:
    print(sys.argv)
    print("Usage: python dump_tar_sys_info.py <tar_sys_info_path> <tar_sys_path> <backup_dir_path> <number of files> <median length of files> <file type set>")
    sys.exit(1)

# Directory path
tar_sys_info_path = sys.argv[1]

# Remove all files in the tar_sys_info directory
file_list = os.listdir(tar_sys_info_path)
if file_list:
    for file_name in file_list:
        # if file_name is directory, remove it recursively
        if os.path.isdir(os.path.join(tar_sys_info_path, file_name)):
            shutil.rmtree(os.path.join(tar_sys_info_path, file_name))
        else:
            os.remove(os.path.join(tar_sys_info_path, file_name))

# Store values in separate files in the tar_sys_info directory
with open(os.path.join(tar_sys_info_path, "tar_sys_path"), "w") as file:
    file.write(sys.argv[2])

with open(os.path.join(tar_sys_info_path, "backup_dir_path"), "w") as file:
    file.write(sys.argv[3])
if len(sys.argv) == 7:
    with open(os.path.join(tar_sys_info_path, "number_of_files"), "w") as file:
        file.write(sys.argv[4])

    with open(os.path.join(tar_sys_info_path, "median_length_of_files"), "w") as file:
        file.write(sys.argv[5])

    with open(os.path.join(tar_sys_info_path, "file_type_set"), "w") as file:
        file.write(sys.argv[6])

    file_type_set_1 = ["txt", "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "jpg", "png", "gif", "mp3", "mp4", "avi", "mov", "wav", "zip", "tar", "gz", "tgz", "bz2", "rar", "7z"]
    with open(os.path.join(tar_sys_info_path, "file_type_set_1"), "w") as file:
        file.write(" ".join(file_type_set_1))

    file_type_set_2 = ["txt", "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx", "jpg", "png", "gif", "mp3", "mp4", "avi"]
    with open(os.path.join(tar_sys_info_path, "file_type_set_2"), "w") as file:
        file.write(" ".join(file_type_set_2))

    file_type_set_3 = ["txt", "pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx"]
    with open(os.path.join(tar_sys_info_path, "file_type_set_3"), "w") as file:
        file.write(" ".join(file_type_set_3))

# dump access
os.system(f"python3 utils/dump_access.py {sys.argv[2]} {tar_sys_info_path}")