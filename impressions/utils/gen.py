import os
import shutil
import sys
def clean_sys(inputfile_path = '../inputfile'):
    with open(inputfile_path, 'r') as f:
        for line in f:
            if line.startswith('Parent_Path:'):
                path = line.split(':')[1].strip().split(' ')[0].strip()
                parent = os.path.dirname(path)
                # check if the parent path exists
                if os.path.exists(parent):
                    shutil.rmtree(parent)
                else:
                    print('Parent path does not exist: {}'.format(parent))
                break

def handle_flags():
    for arg in sys.argv[1:]:
        if arg.startswith("--clean"):
            # get the path of current file
            path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
            clean_sys(path + '/inputfile')
            sys.exit(0)
            
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
    config += "sizebin 0\n"
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
handle_flags()

parent_path = "/home/h/haha/haha 1"
actual_logfile = "./logs 1"
randseeds = [10, 100000, 200000, 300000, 400000, 500000, 600000, 700000, 800000, 900000, 100000]
fs_capacity = "100 GB"
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

config_file = generate_config_file(parent_path, actual_logfile, randseeds, fs_capacity, fs_used, num_files, num_dirs, files_per_dir, \
    file_size_distr, file_count_distr, dir_count_files, dir_size_subdir_distr, files_with_depth, layout_score, actual_file_creation, \
        special_flags, flat, deep, ext, wordfreq, large2small, small2large, depthwithcare, filedepthpoisson, dircountfiles, constraint, printwhat)

path = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
config_path = path + '/config'

# dump the string to config file
with open(config_path, 'w') as f:
    f.write(config_file)
    
cmd = "cd .. && ./impressions config"

os.system(cmd)