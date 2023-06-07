import os
import random
import string
import sys
import shutil
from tqdm import tqdm

from access import make_assignment, clean_up_groups_users

ASCII_SET = string.ascii_letters + string.digits + string.punctuation

def hash(x):
    """A very simple hash function that returns a number between 0 and len(ASCII_SET) - 1

    Args:
        x : a key, which is a int
    """
    return x % len(ASCII_SET)

# Check if correct input format is provided
if len(sys.argv) != 6:
    print("Usage: python generate_tar_sys.py <number of files> <median length of files> <file type set> <output directory> <framework directory>")
    sys.exit(1)

# Create the output directory if it doesn't exist
if not os.path.exists(sys.argv[4]):
    os.makedirs(sys.argv[4])

# Delete all files in the output directory
file_list = os.listdir(sys.argv[4])
if file_list:
    for file_name in file_list:
        os.remove(os.path.join(sys.argv[4], file_name))

# Define file type sets
file_type_set_1 = ['txt', 'pdf', 'doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx', 'jpg', 'png', 'gif', 'mp3', 'mp4', 'avi', 'mov', 'wav', 'zip', 'tar', 'gz', 'tgz', 'bz2', 'rar', '7z']
file_type_set_2 = ['txt', 'pdf', 'doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx', 'jpg', 'png', 'gif', 'mp3', 'mp4', 'avi']
file_type_set_3 = ['txt', 'pdf', 'doc', 'docx', 'xls', 'xlsx', 'ppt', 'pptx']

# Check if the file type set is valid
file_type_set = int(sys.argv[3])
if file_type_set not in [1, 2, 3]:
    print("Invalid file type set, please choose from 1, 2, 3")
    sys.exit(1)

# Check if the number of files is valid
num_files = int(sys.argv[1])
if num_files < 1 or num_files > 10000:
    print("Invalid number of files, please choose from 1 to 10000")
    sys.exit(1)

# Check if the median length of files is valid
median_length = int(sys.argv[2])
if median_length < 1 or median_length > 100000:
    print("Invalid median length of files, please choose from 1 to 100000")
    sys.exit(1)

# Check if the total length of files is less than 1GB
if num_files * median_length > 1000000000:
    print("Total length of files should be less than 1GB")
    sys.exit(1)

file_names = []

# Generate file names
for i in range(num_files):
    if file_type_set == 1:
        file_type = random.choice(file_type_set_1)
    elif file_type_set == 2:
        file_type = random.choice(file_type_set_2)
    else:
        file_type = random.choice(file_type_set_3)
    file_name = f"{file_type}_{i}.{file_type}"
    file_names.append(file_name)

# Create the files in the output directory
with tqdm(total=num_files) as pbar:
    for i, file_name in enumerate(file_names):
        # Generate file size
        file_size = int(median_length * (0.5 + random.random() * 0.5))
        # Generate file content
        file_content = ''.join(ASCII_SET[hash(i)] for i in range(file_size))
        # Create the file
        with open(os.path.join(sys.argv[4], file_name), 'w') as file:
            file.write(file_content)
        # Update the progress bar
        pbar.update(1)

# After having heirarchical structure, we need to set up the number of user groups and different users and assign them to different files and directories
tar_sys_path = sys.argv[4]
framework_dir = sys.argv[5]


def access_create():
    """ Old version, no longer used
    """
    num_groups = int(sys.argv[6])
    num_users = int(sys.argv[7])
    # if access directory does not exist, create it
    if not os.path.isdir(os.path.join(framework_dir, "access")):
        os.mkdir(os.path.join(framework_dir, "access"))
    else:
        # Otherwise we need to do clean_up_groups_users
        clean_up_groups_users(framework_dir)

    # We create the user groups
    for i in range(num_groups):
        # print the group to framework_dir/access/groups
        with open(os.path.join(framework_dir, "access/groups"), "a+") as file:
            file.write(f"group_{i}\n")

    # We create the users
    for i in range(num_groups):
        for j in range(num_users):
            # print the user to framework_dir/access/users
            with open(os.path.join(framework_dir, "access/users"), "a+") as file:
                file.write(f"user_{i}_{j}\n")

    # We create a file set that contains all the file in tar_sys_path
    file_set = []
    for root, dirs, files in os.walk(tar_sys_path):
        for file in files:
            file_set.append(os.path.relpath(os.path.join(root, file), framework_dir))

    print(f"Number of files in {tar_sys_path}: {len(file_set)}")

    num_permissions = int(sys.argv[8])
    print(f"Number of permissions: {num_permissions}")

    # We randomly select num_permissions number of permissions


    # Then we need to dump the assignment of users and groups to files and directories to framework_dir/access/assignment 
    # The format should be <relative path to tar0>,<relative path to tar0>,...  <group0>(r/w/x/...),<group1>(r/w/x/...), ... <user0>(r/w/x/...),<user1>(r/w/x/...), ...

    # We iterate through all files and directories in the target system, and assign users and groups to them
    for it in range(num_permissions):
        groups = ""
        groups_with_permissions = ""
        users = ""
        users_with_permissions = ""
        if num_groups:
            # We randomly select 1 ~ num_groups groups
            groups = ",".join([f"group_{random.randint(0, num_groups - 1)}" for _ in range(random.randint(1, num_groups))])
            # for each group and each user we randomly select r/w/x/rw/rx/wx/rwx for each file so the final format is
            # <relative path to tar> <group0>(r/w/x/...),<group1>(r/w/x/...), ... <user0>(r/w/x/...),<user1>(r/w/x/...), ...
            for group in groups.split(","):
                groups_with_permissions += f"{group}({random.choice(['r', 'w', 'x', 'rw', 'rx', 'wx', 'rwx'])}),"
            # remove the last comma
            groups_with_permissions = groups_with_permissions[:-1]
        if num_users:
            # We randomly select 1 ~ num_users users
            users = ",".join([f"user_{random.randint(0, num_groups - 1)}_{random.randint(0, num_users - 1)}" for _ in range(random.randint(1, num_users))])
            # for each group and each user we randomly select r/w/x/rw/rx/wx/rwx for each file so the final format is
            # <relative path to tar> <group0>(r/w/x/...),<group1>(r/w/x/...), ... <user0>(r/w/x/...),<user1>(r/w/x/...), ...
            for user in users.split(","):
                users_with_permissions += f"{user}({random.choice(['r', 'w', 'x', 'rw', 'rx', 'wx', 'rwx'])}),"
            # remove the last comma
            users_with_permissions = users_with_permissions[:-1]
        if len(file_set):
            # We randomly select 1 ~ len(file_set) files, and assign the users and groups to them, then remove them from file_set
            num_files = random.randint(1, len(file_set))
            files = ",".join([file_set.pop(random.randint(0, len(file_set) - 1)) for _ in range(num_files)])

            # We write the assignment to framework_dir/access/assignment
            with open(os.path.join(framework_dir, "access/assignment"), "a+") as file:
                file.write(f"{files} {groups_with_permissions} {users_with_permissions}\n")



    make_assignment(framework_dir, os.path.join(framework_dir, "access/assignment"))

    # We dump the number of groups and users to framework_dir/access/num_groups and framework_dir/access/num_users
    with open(os.path.join(framework_dir, "access/num_groups"), "w") as file:
        file.write(str(num_groups))

    with open(os.path.join(framework_dir, "access/num_users"), "w") as file:
        file.write(str(num_users))


