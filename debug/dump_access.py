# # This file dumps access related information in target system
# # The input is the path to the target system, as well as the path to the directory where the dumped files will be stored
# import os
# import shutil
# import sys

# # Check if number of arguments is correct
# if len(sys.argv) != 3:
#     print("Usage: python3 dump_access.py <tar_sys_path> <tar_sys_info_path>")
#     sys.exit(1)

# tar_sys_path = sys.argv[1]
# tar_sys_info_path = sys.argv[2]

# # access related files are in tar_sys_info_path/access directory
# access_dir = os.path.join(tar_sys_info_path, "access")

# # Create access directory if not exists
# if not os.path.isdir(access_dir):
#     os.mkdir(access_dir)

# groups_and_permissions = []
# users_and_permissions = []
# # We iterate through all files in target system, and dump access related information to corresponding files
# for root, dirs, files in os.walk(tar_sys_path):
#     for file in files:
#         file_path = os.path.join(root, file)
#         # first dump the file path to both access/groups and access/users
#         with open(os.path.join(access_dir, "groups"), "a+") as file:
#             file.write(f"{file_path}\n")
#         with open(os.path.join(access_dir, "users"), "a+") as file:
#             file.write(f"{file_path}\n")
#         # print(f"Dumping access information of {file_path}")
#         # We dump the groups and their permissions from ACL to access/groups
#         os.system(f"getfacl {file_path} | grep -Eo '^group:[^:]+:[^:]+' | sed 's/^group://' >> {access_dir}/groups")
#         # We dump the users and their permissions from ACL to access/users
#         os.system(f"getfacl {file_path} | grep -Eo '^user:[^:]+:[^:]+' | sed 's/^user://' >> {access_dir}/users")

