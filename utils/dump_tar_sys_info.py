import os
import sys
import shutil


def dump_sys_info(tar_sys_path, dump_file_path):
    """
    Given a set of files in tar_sys_path, we iterate through all files recursively within the tar_sys_path directory,
    then we dump the number of files and the total size of the files in the first line to dump_file_path, and dump following lines in the format :
    <file_path> <file_size>
    So in general, the total format of dump_file_path is:
    <number_of_files> <total_size>
    <file_path_1> <file_size_1>
    <file_path_2> <file_size_2>
    ...
    """
    # Initialize the file count and total size
    file_count = 0
    total_size = 0

    # Open the dump file for writing
    with open(dump_file_path, "w") as dump_file:
        # Iterate through all files recursively within the tar_sys_path directory
        for root, dirs, files in os.walk(tar_sys_path):
            for file_name in files:
                # Get the full path of the file
                file_path = os.path.join(root, file_name)

                # Get the size of the file in bytes
                file_size = os.path.getsize(file_path)

                # Write the file path and size to the dump file
                dump_file.write(f"{file_path} {file_size}\n")

                # Increment the file count and total size
                file_count += 1
                total_size += file_size

    # Write the file count and total size to the beginning of the dump file
    dump_file_contents = f"{file_count} {total_size}\n"
    with open(dump_file_path, "r+") as dump_file:
        contents = dump_file.read()
        dump_file.seek(0, 0)
        dump_file.write(dump_file_contents + contents)
    

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
    dump_sys_info(sys.argv[2], os.path.join(tar_sys_info_path, "tar_sys_dump"))

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
# os.system(f"python3 utils/dump_access.py {sys.argv[2]} {tar_sys_info_path}")