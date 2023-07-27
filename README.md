# Ransomware Testing Framework


## Project Code Basic Structure

```
.
├── config.py
├── core
├── debug
├── docs
├── f2fs-tools-1.16.0
├── img
├── imgs
├── impressions
├── install.sh
├── linux
├── Makefile
├── README.md
├── requirements.txt
├── run.py
└── utils
```

* `config.py` configurations
* `debug` debug files
* `docs` documents
* `f2fs-tools-1.16.0` f2fs-tools
* `img` images
* `imgs` drawio src files
* `impressions` a module used to generate storage system's image for ransomware
* `install.sh` installation script
* `linux` linux kernel containing a part of tracing
* `Makefile` makefile for core functions
* `requirements.txt` python requirements
* `run.py` wrapper for running our framework
* `utils` utilities
  * `core` core functions
    * Running the trace
    * Maintaining a rb tree to record the recoverability
    * Log files (refer to `config.py`)
  * `cryptosoft` ransomware
  * `preprocess` preprocessing testing files
  * `syscall` get blk2file mapping (source code & binaries in bin/ folder)
  * `toplevel` top level functions (called by `run.py`)


### Configurations

All configurations are in `config.py` file.

`config.py` files are in 2 parts. 

* Paths to different files (using absolute paths)
* Functions to get configurations (see `main` function to call them)

In order to use config files, you need to first edit `framework_dir = "/home/h/rans_test`, 
replacing `h` with your username. 

The testing space is under this `framework_dir` folder.

#### `framework_dir` structure

```
.
├── blktrace
├── logs
|   ├── logs_100001
|   ├── logs_100002
|   ├── ...
|   ├── test_dir_path
|   ├── test_id
|   └── trace_path
├── rans_config
|   ├── rans_repos
|   ├── rans_tested
|   ├── sys_repos
|   └── sys_tested
└── tar_sys
    ├── 1/
    ├── 3/
    ├── ...
    ├── injected/
    └── lots_of_files.ext
```

### Calling Trace

* `run.py` wrapper
* `utils/toplevel.py` initiate testing
* kernel level tracing
* `utils/core/*.cpp` Use a rb tree to record # clean blocks
* `utils/preproecess.py` initiate ransomware and do `blktrace` and `blkparse`
* `utils/cryptosoft/ransomware.py` run ransomware
* `utils/core/*cpp` Update rb tree to calcuate final result (# clean blocks remaining)
* `utils/toplevel.py` initiate another test

#### Procedure

To test local storage system, we conduct the following steps:
* Creating file system image. By file system image, according to this [paper](https://www.usenix.org/legacy/events/fast09/tech/full_papers/agrawal/agrawal.pdf), we mean the a state combining in-memory and on-disk layout of a storage system. For instanace, the status of the cache and how fragmented the files are in disk both contribute to the file system image. 
* Inject testing files into the system. Because original file system image is too large and might be sensitive to work on, it's important to only operate on the files that we injected to obtain insights. In order to better draw conclusions from the injected files, the injected files need to be representitive and diverse.
* Obtain file2blk & blk2file mappings to gain a set of blocks that relate to the injected files.
* Maintain a Red-Black Tree to record these blocks. Originally, these blocks are marked as clean / recoverable.
* Run a version of ransomware against the injected files. Record the block trace in background.
* Analyze the block trace. If a block to be encrypted is associated with a block related to injected files, we find the corresponding node on Red-Black Tree and we mark it as unrecoverable.
* Finally we calculate the recoverability of the storage system by looking at the ratio between `# clean block after ransomware attack` and `# clean blocks before ransomware attack`.

During the process above, only the blocks associated with injected files are checked and no disk-scan is taking place.

We automatically change the injected files' pattern and the ransomware version (by changing a set of parameters) to obtain different recoverability.

Eventually, we obtain a set of $\text{parameters} \rightarrow \text{recoverability}$ mapping. We use [Decision Tree](https://scikit-learn.org/stable/modules/tree.html) to obtain the relation between `version of ransomware` and `the recoverability`, and we can also know the relation between injected files' pattern and recoverability.