import os
import shutil
import sys


def rm_groups(output_dir):
    # remove all groups recorded in <output_directory>/access/groups from linux
    with open(os.path.join(output_dir, "access/groups"), "r") as file:
        for line in file:
            group = line.strip()
            os.system(f"sudo groupdel {group}")

def rm_users(output_dir):
    # remove all users recorded in <output_directory>/access/users from linux
    with open(os.path.join(output_dir, "access/users"), "r") as file:
        for line in file:
            user = line.strip()
            # stop the mail pool not found error message
            os.system(f"sudo userdel -r {user} 2> /dev/null")

def clean_up_groups_users(output_dir):
    # if users file exists, remove it
    if os.path.isfile(os.path.join(output_dir, "access/users")):
        rm_users(output_dir)
        os.remove(os.path.join(output_dir, "access/users"))
    # if groups file exists, remove it
    if os.path.isfile(os.path.join(output_dir, "access/groups")):
        # remove all groups and users recorded in <output_directory>/access/groups and <output_directory>/access/users from linux
        rm_groups(output_dir)
        # remove the groups and users file
        os.remove(os.path.join(output_dir, "access/groups"))

def make_assignment(framework_dir_path, assignment_path):
    '''
    This function makes the assignment of users and groups (contained in assignment file given by assginment path) to files and directories.
    The assignment path is output/access/assignment and the format of the file is
    <relative of file0 path to tar0>,<relative of file1 path to tar1>,... <group0>(r/w/x/...),<group1>(r/w/x/...),... <user0>(r/w/x/...),<user1>(r/w/x/...),...
    <relative of file0 path to tar0'>,<relative of file1 path to tar1'>,... <group0'>(r/w/x/...),<group1'>(r/w/x/...),... <user0'>(r/w/x/...),<user1'>(r/w/x/...),...
    Assignment means we assign the users and groups with theirto the files and directories in the target system
    ...
    '''
    print(f"Making assignment to {framework_dir_path} from {assignment_path}")
    import time
    # We iterate through all files and directories the assignment file, and assign users and groups to them
    with open(assignment_path, "r") as file:
        for line in file:
            # Each line the format is <relative path to tar> <group0>(r/w/x/...),<group1>(r/w/x/...), ... <user0>(r/w/x/...),<user1>(r/w/x/...), ...
            line = line.strip()
            if len(line.split(" ")) != 3:
                print(f"Invalid line: {line}")
                continue
            relative_path, groups, users = line.split(" ", 2) 
            # print(f"Assigning {groups} and {users} to {relative_path}")
            # We split the groups and users by comma
            groups = groups.split(",")
            users = users.split(",")

            start_time = time.time()

            # Then we first check if the user / group exists, if not, we create them
            if len(groups):
                for group in groups:
                    # first extract the group name
                    group_name = group.split("(")[0]
                    # check if the group exists
                    if os.system(f"getent group {group_name} > /dev/null") != 0:
                        # if not, create the group with sudo
                        os.system(f"sudo groupadd {group_name}")
            if len(users):
                for user in users:
                    # first extract the user name
                    user_name = user.split("(")[0]
                    # check if the user exists
                    if os.system(f"getent passwd {user_name} > /dev/null") != 0:
                        # if not, create the user without password with sudo
                        os.system(f"sudo useradd {user_name}")

            print(f"Time to create users and groups: {time.time() - start_time}")

            start_time = time.time()
            
            # Then we assign the users and groups to the file / directory
            # We first get the absolute path to the file / directory
            fileset = relative_path.split(",")
            abs_path_set = [os.path.join(framework_dir_path, file) for file in fileset]
            abs_path = " ".join(abs_path_set)

            if len(groups):
                # We set the permissions for all the groups in one command
                # with the command e.g. setfacl -m u:user1:rw,u:user2:r,u:user3:rx,... myfile.txt
                group_names = [group.split("(")[0] for group in groups]
                group_permissions = [group.split("(")[1][:-1] for group in groups]
                group_name_and_permissions = ",".join([f"g:{group_names[i]}:{group_permissions[i]}" for i in range(len(group_names))])
                os.system(f"sudo setfacl -m {group_name_and_permissions} {abs_path}")
            if len(users):
                # We set the permissions for all the users in one command
                # with the command e.g. setfacl -m u:user1:rw,u:user2:r,u:user3:rx,... myfile.txt
                user_names = [user.split("(")[0] for user in users]
                user_permissions = [user.split("(")[1][:-1] for user in users]
                user_name_and_permissions = ",".join([f"u:{user_names[i]}:{user_permissions[i]}" for i in range(len(user_names))])
                os.system(f"sudo setfacl -m {user_name_and_permissions} {abs_path}")
            print(f"Time to assign users and groups: {time.time() - start_time}")
            

