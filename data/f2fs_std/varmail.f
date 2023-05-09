Filebench Version 1.5-alpha3
0.000: Allocated 177MB of shared memory
0.014: Varmail Version 3.0 personality successfully loaded
0.014: Populating and pre-allocating filesets
0.092: bigfileset populated: 100000 files, avg. dir. width = 1000000, avg. dir. depth = 0.8, 0 leafdirs, 3126.206MB total size
0.092: Removing bigfileset tree (if exists)
0.095: Pre-allocating directories in bigfileset tree
0.095: Pre-allocating files in bigfileset tree
3.259: Waiting for pre-allocation to finish (in case of a parallel pre-allocation)
3.259: Population and pre-allocation of filesets completed
3.260: Starting 1 filereader instances
4.269: Running...
64.275: Run took 60 seconds...
64.278: Per-Operation Breakdown
closefile4           66995ops     1117ops/s   0.0mb/s    0.002ms/op [0.000ms - 0.037ms]
readfile4            66995ops     1117ops/s  28.9mb/s    0.021ms/op [0.002ms - 4.963ms]
openfile4            66995ops     1117ops/s   0.0mb/s    0.028ms/op [0.003ms - 4.264ms]
closefile3           66995ops     1117ops/s   0.0mb/s    0.004ms/op [0.001ms - 0.035ms]
fsyncfile3           66995ops     1117ops/s   0.0mb/s   22.438ms/op [0.240ms - 143.475ms]
appendfilerand3      67022ops     1117ops/s   8.7mb/s    0.535ms/op [0.003ms - 46.521ms]
readfile3            67022ops     1117ops/s  28.6mb/s    0.020ms/op [0.002ms - 1.020ms]
openfile3            67022ops     1117ops/s   0.0mb/s    0.026ms/op [0.003ms - 4.164ms]
closefile2           67022ops     1117ops/s   0.0mb/s    0.004ms/op [0.001ms - 0.059ms]
fsyncfile2           67023ops     1117ops/s   0.0mb/s   20.723ms/op [0.036ms - 512.685ms]
appendfilerand2      67045ops     1117ops/s   8.7mb/s    0.021ms/op [0.004ms - 7.176ms]
createfile2          67045ops     1117ops/s   0.0mb/s    0.349ms/op [0.032ms - 111.749ms]
deletefile1          67045ops     1117ops/s   0.0mb/s    0.390ms/op [0.017ms - 117.290ms]
64.278: IO Summary: 871221 ops 14519.319 ops/s 2233/2234 rd/wr  74.9mb/s 3.427ms/op
64.278: Shutting down processes