import os
import shutil
import sys

import threading
from queue import Queue

config_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

sys.path.append(config_dir)

from crypto_config import ACCESS_TYPE, GEN_CONFIG


class Chunk:
    def __init__(self, filename, loff, length, data=None):
        self.filename = filename
        self.loff = loff
        self.length = length
        self.data = data


class ChunkSet:
    """
    A class representing a set of file chunks. The ChunkSet is indexed by filename and each filename is mapped to a list of chunks.
    """
    def __init__(self):
        self.chunks = {}
        self.lock = threading.Lock()

    def insert(self, chunk):
        with self.lock:
            if chunk.filename not in self.chunks:
                self.chunks[chunk.filename] = []
            self.chunks[chunk.filename].append(chunk)

    def pop(self, filename):
        """
        Pop the first chunk from the list of chunks for the given filename.
        """
        with self.lock:
            if filename in self.chunks and len(self.chunks[filename]) > 0:
                return self.chunks[filename].pop(0)
            else:
                return None
            
    def pop_all(self, filename):
        """
        Pop all chunks from the list of chunks for the given filename.
        """
        with self.lock:
            if filename in self.chunks and len(self.chunks[filename]) > 0:
                all_chunks = self.chunks[filename]
                # remove the filename from the chunk_set
                del self.chunks[filename]
                return all_chunks
            else:
                return None
        
    def encrpt(self, filename):
        """
        Encrypt the first chunk from the list of chunks for the given filename.
        """
        with self.lock:
            if filename in self.chunks and len(self.chunks[filename]) > 0:
                for chunk in self.chunks[filename]:
                    # encrypting here is just make byte = (~byte) & 0xff
                    chunk.data = bytes([(~byte) & 0xff for byte in chunk.data])
            else:
                return None

    def delete(self, filename):
        with self.lock:
            if filename in self.chunks:
                del self.chunks[filename]
    def dump_pick(self, M = 2, N = 2):
        """
        Pick M filename randomly in the chunk_set and dump its (at most N) chunks
        """
        # the filenames are in self.chunks.keys()
        filenames = list(self.chunks.keys())
        num_dumped = 0
        # randomly pick M filenames
        import random
        random.shuffle(filenames)
        for filename in filenames:
            if num_dumped >= M:
                break
            self.dump(filename, N)
            num_dumped += 1
        
    def dump(self, filename, N = 2):
        """
        Dump the chunks with the given filename to a file using print
        """
        num_dumped = 0
        with self.lock:
            if filename in self.chunks.keys():
                print(f"Dumping chunks for {filename}")
                for chunk in self.chunks[filename]:
                    print(f"loff={chunk.loff} length={chunk.length}")
                    print(chunk.data)
                    if num_dumped >= N:
                        break
                    num_dumped += 1
            else:
                return None
        

def read_files(tar_sys_path):
    """
    Read files from tar_sys_path and insert them into chunk_set which is a class consisting of a dictionary of chunks.
    The ChunkSet is indexed by filename and each filename is mapped to a list of chunks.
    """
    chunk_size = GEN_CONFIG["chunk_size"]
    chunk_set = ChunkSet()
    

    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            # Get the file length first
            flen = os.path.getsize(os.path.join(root, filename))
            
            filepath = os.path.join(root, filename)
            with open(filepath, "rb") as f:
                loff = 0
                while True:
                    chunk = f.read(min(chunk_size, flen - loff))
                    if not chunk:
                        break
                    chunk_set.insert(Chunk(filepath, loff, len(chunk), chunk))
                    loff += len(chunk)

    return chunk_set

def read_whole_file():
    """
    Basically the same as read_files() but reads the whole file at once(then divide into chunks in memory) instead of in chunks.
    """
    chunk_size = GEN_CONFIG["chunk_size"]
    chunk_set = ChunkSet()

    for root, dirs, files in os.walk(GEN_CONFIG["tar_sys_path"]):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "rb") as f:
                file_contents = f.read()
                loff = 0
                while loff < len(file_contents):
                    chunk = file_contents[loff:loff+chunk_size]
                    chunk_set.insert(Chunk(filepath, loff, len(chunk), chunk))
                    loff += len(chunk)
    return chunk_set

def read_files_threaded():
    chunk_size = GEN_CONFIG["chunk_size"]
    chunk_set = ChunkSet()

    # Create a queue to hold the filenames
    filename_queue = Queue()

    # Add all the filenames to the queue
    for root, dirs, files in os.walk(GEN_CONFIG["tar_sys_path"]):
        for filename in files:
            filepath = os.path.join(root, filename)
            filename_queue.put(filepath)

    # Create a worker function to read files and insert chunks into the ChunkSet
    def worker():
        while True:
            # Get the next filename from the queue
            filepath = filename_queue.get()
            if filepath is None:
                break

            # Read the file contents into memory
            with open(filepath, "rb") as f:
                file_contents = f.read()

            # Split the file contents into chunks and insert them into the ChunkSet
            loff = 0
            while loff < len(file_contents):
                chunk = file_contents[loff:loff+chunk_size]
                chunk_set.insert(Chunk(filepath, loff, len(chunk), chunk))
                loff += len(chunk)

            # Mark the filename as done
            filename_queue.task_done()

    # Create a pool of worker threads
    num_threads = GEN_CONFIG["num_threads"]
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)

    # Wait for all the filenames to be processed
    filename_queue.join()

    # Stop the worker threads
    for i in range(num_threads):
        filename_queue.put(None)
    for t in threads:
        t.join()

    return chunk_set

def encrpt_all(chunk_set):
    """
    Encrypt all the files in tar_sys_path by calling encrypt function in ChunkSet class.
    """
    for root, dirs, files in os.walk(GEN_CONFIG["tar_sys_path"]):
        for filename in files:
            filepath = os.path.join(root, filename)
            chunk_set.encrpt(filepath)

def write_files(chunk_set, tar_sys_path):
    """
    Write the encrypted chunks in chunk_set back to tar_sys_path.
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "wb") as f:
                while True:
                    chunk = chunk_set.pop(filepath)
                    if not chunk:
                        break
                    f.seek(chunk.loff)
                    f.write(chunk.data)

def write_whole_files(chunk_set, tar_sys_path):
    """
    For each file, we first assemble the chunks then write the whole file back to disk.
    """
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "wb") as f:
                chunks = chunk_set.pop_all(filepath)
                chunks.sort(key=lambda x: x.loff)
                # assemble data
                data = []
                for chunk in chunks:
                    data.append(chunk.data)

                data = b"".join(data)
                f.write(data)

def write_files_threaded(chunk_set):
    """
    Write the encrypted chunks in chunk_set back to tar_sys_path in parallel.
    """
    # Create a queue to hold the filenames
    filename_queue = Queue()

    # Add all the filenames to the queue
    for root, dirs, files in os.walk(GEN_CONFIG["tar_sys_path"]):
        for filename in files:
            filepath = os.path.join(root, filename)
            filename_queue.put(filepath)

    # Create a worker function to write files
    def worker():
        while True:
            # Get the next filename from the queue
            filepath = filename_queue.get()
            if filepath is None:
                break

            # Write the file contents to disk
            with open(filepath, "wb") as f:
                while True:
                    chunk = chunk_set.pop(filepath)
                    if not chunk:
                        break
                    f.seek(chunk.loff)
                    f.write(chunk.data)

            # Mark the filename as done
            filename_queue.task_done()

    # Create a pool of worker threads
    num_threads = GEN_CONFIG["num_threads"]
    threads = []
    for i in range(num_threads):
        t = threading.Thread(target=worker)
        t.start()
        threads.append(t)

    # Wait for all the filenames to be processed
    filename_queue.join()

    # Stop the worker threads
    for i in range(num_threads):
        filename_queue.put(None)
    for t in threads:
        t.join()


def flush_sync_files(tar_sys_path):
    """
    Flush the files in tar_sys_path to disk by calling f.flush then os.sync.
    """
    import time
    flush_start = time.time()
    for root, dirs, files in os.walk(tar_sys_path):
        for filename in files:
            filepath = os.path.join(root, filename)
            with open(filepath, "rb+") as f:
                f.flush()
    os.sync()
    print(f"Flush time: {time.time() - flush_start} seconds")
    print("Flushed all files to disk.")

def main():
    tar_sys_path = GEN_CONFIG["tar_sys_path"]
    chunk_set = read_files(tar_sys_path)
    encrpt_all(chunk_set)
    # chunk_set.dump_pick()
    write_whole_files(chunk_set, tar_sys_path)
    flush_sync_files(tar_sys_path)


if __name__ == '__main__':
    main()