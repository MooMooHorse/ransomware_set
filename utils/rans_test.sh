# This script runs the ransomware testing framework on the target system specified by one of the input.
# The input format is as follows
# ./rans_test.sh <tar_sys_directory> <ransomware_path> <backup_dir_path>

# The script will create a directory named "rans_test" in the current directory and will store the results in that directory.
# a list of set paths
tar_sys_info_path="rans_test/tar_sys_info"
duplicate_dir="rans_test/duplicates"

# check if we're in ransomware_set/utils directory
if [ ! -d "../utils" ]
then
    echo "Please run this script in ransomware_set/utils directory"
    exit
fi

cd ../
# check if rans_test directory exists
if [ ! -d "rans_test" ]
then
    mkdir rans_test
fi

# create tar_sys_info directory if not exists
if [ ! -d $tar_sys_info_path ]
then
    mkdir $tar_sys_info_path
fi

# Assign default value to inputs
tar_sys_path="rans_test/tar_sys"
rans_path="rans_test/ransomware"
backup_dir_path="rans_test/backup_dir"

# Parse command-line arguments
for arg in "$@"; do
  case "$arg" in
    -t=*|--tar_sys_path=*) tar_sys_path="${arg#*=}" ;;
    -r=*|--rans_path=*) rans_path="${arg#*=}" ;;
    -b=*|--backup_dir_path=*) backup_dir_path="${arg#*=}" ;;
    *) echo "Invalid flag: $arg"; exit 1 ;;
  esac
done

# check if target system exists, if not ask user to input <number of files> <median length of files> <file type set>, then in this script we 
# run ./generate_tar_sys.sh <number of files> <median length of files> <file type set> <output directory = $tar_sys_path> to generate the target system
if [ ! -d $tar_sys_path ]
then
    echo "Target system does not exist, please input <number of files> <median length of files> <file type set> to generate the target system"
    read number_of_files median_length_of_files file_type_set
    python3 utils/gen_tar_sys.py $number_of_files $median_length_of_files $file_type_set $tar_sys_path
fi

# run dump_tar_sys_info.py <tar_sys_info_path> <tar_sys_path> <backup_dir_path> <number of files> <median length of files> <file type set>
# This will dump these info to corresponding files in tar_sys_info_path
python3 utils/dump_tar_sys_info.py $tar_sys_info_path $tar_sys_path $backup_dir_path $number_of_files $median_length_of_files $file_type_set

# This will duplicate 
python3 utils/mkdup.py $duplicate_dir $tar_sys_path $backup_dir_path


# tell user where is backup is stored and confirm their input
echo "Backup of target system is stored in $duplicate_dir/duplicate_tar_sys"
echo "Backup of backup directory is stored in $duplicate_dir/duplicate_backup_dir"
echo "Do you want to continue? with tar_sys_path = $tar_sys_path, rans_path = $rans_path, backup_dir_path = $backup_dir_path (y/n)"
read input
if [ $input == "n" ]
then
    exit
fi


