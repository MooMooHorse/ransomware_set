#!/bin/bash
#HOME=/big_disk/puneet/impress_home/
HOME=/home/puneet/
#Fig a : directories by namespace depth
for i in `seq 1 16`;
do
    echo -n $i","
    find $HOME -maxdepth $i -mindepth $i -type d | wc -l
    # echo
    # find $HOME -mindepth $i -maxdepth $i -type d -printf "%d %p\n" | wc -l
done



# #FIg b: directories by Subdirectory count. 
# for i in $(find $HOME -maxdepth 16 -mindepth 1 -type d) ; do 
#     echo -n $(find $i -mindepth 1 -maxdepth 1 -type d | wc -l) "," $i;
#     echo
# done


# #fig c: files by size

# find $HOME -type f -exec ls -l '{}' \;| awk '{printf "%s,%s\n", $5, $9} '

# # #fig d : files by containing bytes

# # find /big_disk/puneet/impress_home/ -type f -exec ls -lh '{}' \;|awk '{printf "%s,%s\n", $5, $9} '

# #top extensions
# do find $HOME -type f  

# f files by namespace depth
# for i in `seq 1 16`;
# do
#     echo $i
#     find $HOME -mindepth $i -maxdepth $i -type f | wc -l
#     echo
# done

# #bytes by namespace depth
# for i in `seq 1 16`;
# do
#     echo $i
#     find $HOME -mindepth $i -maxdepth $i -type f -exec ls -l '{}' \;| awk '{printf "%s,%s\n", $5,$9} '
#     echo
# done
