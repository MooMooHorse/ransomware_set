---
marp : true
---
<style>
img[alt~="center"] {
  display: block;
  margin: 0 auto;
}
</style>

# Testing Local Storage System

---

## Review : How to test local storage system

---


1. Creating file system image. A real storage system originally has a set of objects stored in disk and in cache. We create different file distribution with different fragmentation degree in disk. This is done via this [paper](https://www.usenix.org/legacy/events/fast09/tech/full_papers/agrawal/agrawal.pdf).

---

2. Inject diverse and representitive testing files into the system. Because original file system image is too large and has sensitive information, it's important to only operate on the files we inject to obtain insights.

---


3. Obtain file2blk & blk2file mappings.

---


4. Maintain a Red-Black Tree to record the blocks associated with injected files. Originally, these blocks are marked as clean / recoverable.

---

5. Run a version of ransomware against the injected files. Record the block trace in the background.


---


6. Analyze the block trace. From blk2file mapping, if a block encrypted is associated with injected files, we find the corresponding node on the Red-Black Tree and mark it as unrecoverable.


---


7. Finally we calculate the recoverability of the storage system by looking at the ratio between `# clean block after ransomware attack` and `# clean blocks before ransomware attack`.


---

# Analysis of results

From the raw data obtained by the previous method, we get $2304$ `ransomware parameter` $\rightarrow$ `recoverability of storage system` mapping for each storage system.


---

## Automated analysis : Decision Tree

It's hard to analyze 2304 mappings manually, because we have ~10 reansomware features, each of which could influence recoverability of storage system.

* e.g. if we only have 1 feature (feature $A$) for a ransomware workload, then we can simply draw a `recoverability` against `feature` plot to see how feature $A$ influence the recoverability. 

To conquer the problem brought by high dimension of input, we utilize the [Decision Tree](https://scikit-learn.org/stable/modules/tree.html) to gain insights.


---

