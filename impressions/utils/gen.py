import os
import shutil
import sys



# default config for the target system

bacth_index = 1
path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
stat_dir = os.path.join(path, 'stat')
config_dir = os.path.join(path, 'config')


parent_path = "/home/h/haha 1"
actual_logfile = f"{path}/logs" # log directory
if not os.path.exists(actual_logfile):
    os.mkdir(actual_logfile)
actual_logfile = actual_logfile + " 1"
    
randseeds = [10, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 100000]
fs_capacity = "200 GB"
fs_used = "10 GB"
num_files = "NO K"
num_dirs = "NO K"
files_per_dir = "10.2 N"
file_size_distr = "Direct 3 99994 29 0.91"
file_count_distr = "Direct 2 9.48 2.45663283"
dir_count_files = "Direct 2 2 2.36"
dir_size_subdir_distr = "Indir DirsizesubdirDistr.txt"
files_with_depth = "Direct 10"
layout_score = "1.0"
actual_file_creation = "1"
special_flags = "10"
flat = 0
deep = 0
ext = -1
wordfreq = 0
large2small = 0
small2large = 0
depthwithcare = 1
filedepthpoisson = 1
dircountfiles = 0
constraint = 0
printwhat = 10



def clean_sys(inputfile_path):
    global config_dir
    if inputfile_path is None:
        # for all config files
        for config_file in os.listdir(config_dir):
            with open(os.path.join(config_dir, config_file), 'r') as f:
                for line in f:
                    if line.startswith('Parent_Path:'):
                        path = line.split(':')[1].strip().split(' ')[0].strip()
                        # check if the parent path exists
                        if os.path.exists(path):
                            shutil.rmtree(path)
                    elif line.startswith('Actuallogfile:'):
                        path = line.split(':')[1].strip().split(' ')[0].strip()
                        if os.path.exists(path):
                            shutil.rmtree(path)
        # remove all config files
        for config_file in os.listdir(config_dir):
            os.remove(os.path.join(config_dir, config_file))
    else:
        # for the specified config file
        with open(inputfile_path, 'r') as f:
            for line in f:
                if line.startswith('Parent_Path:'):
                    path = line.split(':')[1].strip().split(' ')[0].strip()
                    # check if the parent path exists
                    if os.path.exists(path):
                        shutil.rmtree(path)
                    else:
                        print('Parent path does not exist: {}'.format(path))
                elif line.startswith('Actuallogfile:'):
                    path = line.split(':')[1].strip().split(' ')[0].strip()
                    if os.path.exists(path):
                        shutil.rmtree(path)
    # remove the stat directory
    stat_path = os.path.join(os.path.dirname(os.path.dirname(os.path.realpath(__file__))), 'stat')
    if os.path.exists(stat_path):
        shutil.rmtree(stat_path)
def handle_flags():
    """
    Handle the flags passed to the script.
    An example would be :
    python3 impressions/utils/gen.py --clean[=config/config_2]
    which removes the 2nd config file in the config directory (all related redundant files, logs, stat, and tar_sys)
    Another example would be :
    rm -rf /home/h/rans_test && mkdir /home/h/rans_test && mkdir /home/h/rans_test/tar_sys && python3 /home/h/ransomware_set/impressions/utils/gen.py -path=/home/h/rans_test/tar_sys -batch=2 -tused=10 -usedunit=GB -mu=4 -fscore=1.0
    whould generate a target system whose size distribution has peak at small files
    rm -rf /home/h/rans_test && mkdir /home/h/rans_test && mkdir /home/h/rans_test/tar_sys && python3 /home/h/ransomware_set/impressions/utils/gen.py -path=/home/h/rans_test/tar_sys -batch=2 -tused=100 -usedunit=GB -mu=17 -fscore=1.0
    """
    global parent_path, bacth_index, fs_used, file_count_distr, layout_score
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            config_name = arg.split('=')
            if len(config_name) == 1:
                config_name = None
            else:
                config_name = config_name[1].strip()
            # get the path of current file
            path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            if config_name is None:
                clean_sys(None)
            else:
                config_path = os.path.join(path, 'config')
                clean_sys(config_path)
                # rm the config file
                if os.path.exists(config_path):
                    os.remove(config_path)
            sys.exit(0)
        elif arg.startswith("-path"):
            parent_path = arg.split('=')[1].strip() + ' 1'
        elif arg.startswith('-batch'):
            bacth_index = int(arg.split('=')[1].strip())
        elif arg.startswith('-tused'):
            fs_used = arg.split('=')[1].strip()
        elif arg.startswith('-usedunit'):
            fs_used = fs_used + ' ' + arg.split('=')[1].strip()
        elif arg.startswith('-mu'):
            file_count_distr = "Direct 2 " + arg.split('=')[1].strip() + " 2.45663283"
        elif arg.startswith('-fscore'):
            layout_score = float(arg.split('=')[1].strip())
            
def generate_config_file(parent_path, actual_logfile, randseeds, fs_capacity, fs_used, num_files, num_dirs, \
    files_per_dir, file_size_distr, file_count_distr, dir_count_files, dir_size_subdir_distr, \
    files_with_depth, layout_score, actual_file_creation, special_flags, flat, deep, ext, wordfreq, \
    large2small, small2large, depthwithcare, filedepthpoisson, dircountfiles, constraint, printwhat):
    config = "Parent_Path: {}\n".format(parent_path)
    config += "Actuallogfile: {}\n".format(actual_logfile)
    config += "Randseeds: Direct {}\n".format(" ".join(map(str, randseeds)))
    config += "FScapacity: {}\n".format(fs_capacity)
    config += "FSused: {}\n".format(fs_used)
    config += "Numfiles: {}\n".format(num_files)
    config += "Numdirs: {}\n".format(num_dirs)
    config += "Filesperdir: {}\n".format(files_per_dir)
    config += "FilesizeDistr: {}\n".format(file_size_distr)
    config += "FilecountDistr: {}\n".format(file_count_distr)
    config += "Dircountfiles: {}\n".format(dir_count_files)
    config += "DirsizesubdirDistr: {}\n".format(dir_size_subdir_distr)
    config += "Fileswithdepth: {}\n".format(files_with_depth)
    config += "Layoutscore: {}\n".format(layout_score)
    config += "Actualfilecreation: {}\n".format(actual_file_creation)
    config += "SpecialFlags: {}\n".format(special_flags)
    config += "Flat {}\n".format(flat)
    config += "Deep {}\n".format(deep)
    config += "Ext {}\n".format(ext)
    config += "Wordfreq {}\n".format(wordfreq)
    config += "Large2Small {}\n".format(large2small)
    config += "Small2Large {}\n".format(small2large)
    config += "Depthwithcare {}\n".format(depthwithcare)
    config += "Filedepthpoisson {}\n".format(filedepthpoisson)
    config += "Dircountfiles {}\n".format(dircountfiles)
    config += "Constraint {}\n".format(constraint)
    config += "Printwhat: {}\n".format(printwhat)
    config += "ext 0\n"
    config += "sizebin 1\n"
    config += "size 0\n"
    config += "initial 0\n"
    config += "final 1\n"
    config += "depth 0\n"
    config += "tree 0\n"
    config += "subdirs 0\n"
    config += "dircountfiles 0\n"
    config += "constraint 0\n"
    config += "SpecialDirBias\n"

    return config

def main():
    global parent_path, bacth_index, fs_used, file_count_distr, layout_score
    
    handle_flags()
    
    if not os.path.exists(config_dir):
        os.mkdir(config_dir)
    config_path = config_dir + f'/config_{bacth_index}'

    config_file = generate_config_file(parent_path, actual_logfile, randseeds, fs_capacity, fs_used, num_files, num_dirs, files_per_dir, \
        file_size_distr, file_count_distr, dir_count_files, dir_size_subdir_distr, files_with_depth, layout_score, actual_file_creation, \
            special_flags, flat, deep, ext, wordfreq, large2small, small2large, depthwithcare, filedepthpoisson, dircountfiles, constraint, printwhat)

    
    # dump the string to config file
    with open(config_path, 'w') as f:
        f.write(config_file)
    
    impression_path = path + '/impressions'
    parent_path = parent_path.split(' ')[0].strip()
    # stat dir 
    if not os.path.exists(stat_dir):
        os.mkdir(stat_dir)
    if not os.path.exists(parent_path):
        os.mkdir(parent_path)
    
    cmd = f"rm -rf {parent_path}/* && {impression_path} {config_path} {bacth_index} > {stat_dir}/stat_{bacth_index}"
    
    os.system(cmd)
    
if __name__ == '__main__':
    main()
