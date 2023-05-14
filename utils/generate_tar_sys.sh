# This script file takes in 
# number of files
# median length of files
# file type set (we have 3 options, each options contains a set of file types)
# output directory

# The script will generate a set of files with the given number, length and type
# and put them in the output directory

# Usage: ./generate_tar_sys.sh <number of files> <median length of files> <file type set> <output directory>

# We first check if user has correct input format
if [ $# -ne 4 ]; then
    echo "Usage: ./generate_tar_sys.sh <number of files> <median length of files> <file type set> <output directory>"
    exit 1
fi

# We then check if the output directory exists, if not, we create one
if [ ! -d $4 ]; then
    mkdir $4
fi

# We then check if the output directory is empty, if not, we delete all files in it with sudo privilege
if [ "$(ls -A $4)" ]; then
    sudo rm -rf $4/*
fi

# then create 3 sets of file types
# set 1 is the most complete set of file types, set 2,3 are subset of set 1 
file_type_set_1=(txt pdf doc docx xls xlsx ppt pptx jpg png gif mp3 mp4 avi mov wav zip tar gz tgz bz2 rar 7z)
file_type_set_2=(txt pdf doc docx xls xlsx ppt pptx jpg png gif mp3 mp4 avi)
file_type_set_3=(txt pdf doc docx xls xlsx ppt pptx)

# We then check if the file type set is valid
if [ $3 -ne 1 ] && [ $3 -ne 2 ] && [ $3 -ne 3 ]; then
    echo "Invalid file type set, please choose from 1, 2, 3"
    exit 1
fi

# We then check if the number of files is valid, number of files should be [1, 10000]
if [ $1 -lt 1 ] || [ $1 -gt 10000 ]; then
    echo "Invalid number of files, please choose from 1 to 10000"
    exit 1
fi

# We then check if the median length of files is valid, median length of files should be [1, 100000]
if [ $2 -lt 1 ] || [ $2 -gt 100000 ]; then
    echo "Invalid median length of files, please choose from 1 to 100000"
    exit 1
fi

# we check if the total length of files should be less than 1GB
if [ $(( $1 * $2 )) -gt 1000000000 ]; then
    echo "Total length of files should be less than 1GB"
    exit 1
fi

file_names=()

# We first generate the file names, we use the format of <file type>_<file number>.<file type>
for (( i=0; i<$1; i++ ))
do
    if [ $3 -eq 1 ]; then
        file_type=${file_type_set_1[$RANDOM % ${#file_type_set_1[@]} ]}
    elif [ $3 -eq 2 ]; then
        file_type=${file_type_set_2[$RANDOM % ${#file_type_set_2[@]} ]}
    else
        file_type=${file_type_set_3[$RANDOM % ${#file_type_set_3[@]} ]}
    fi
    file_name=$file_type"_"$i"."$file_type
    file_names[$i]=$file_name
done


# we create the files' in file_names

for file_name in "${file_names[@]}"
do
    # we first generate the file size
    # we use the formula: file size = median length * (0.5 + 0.5 * random number)
    file_size=$(( $2 * ( 50 + $RANDOM % 51 ) / 100 ))
    # we then generate the file content
    # we use the formula: file content = random string with length = file size
    file_content=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $file_size | head -n 1)
    # we then create the file
    echo $file_content > $4/$file_name
done



