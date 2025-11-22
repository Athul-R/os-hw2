
# Mutex
### Q0
> Q: What circumstances cause an entry to get lost? Analyze the initial code and write a short answer to describe what it means for an entry to be “lost,” and which parts of the program are causing this unintended behavior when run with multiple threads.

Entries are “lost” when a key that was scheduled to be inserted can’t be found later; i.e., retrieve() returns NULL even though its thread called insert() for that key.



insert() performs a classic head insertion into the shared bucket list with no synchronization. 
The **parallel_hashtable.c** program uses only five buckets in its hash table and attempts to insert **100,000 entries**. It takes the number of threads as a command-line argument and then launches that many threads so they can perform the insertions in parallel. After all insertion threads finish, the program spawns an equal number of threads to retrieve those entries concurrently.

During insertion, every thread writes to the **same shared hash table**, which means each thread updates the same underlying data structure. Because these threads operate without proper synchronization, they trigger **race conditions**, and some entries never make it into the hash table. The root cause of this incorrect behaviour is the **insert()** function, which accesses the shared resource:

![Keys Lost](./images/keys_lost.png)

```C
bucket_entry * table[NUM_BUCKETS];
```

Without locking, multiple threads modify the same buckets at the same time, leading to lost or overwritten entries.

Since the program retrieves the entries after the insert is done, the multiple threads of
retrieval are not concurrent with insert threads. Therefore, the retrieval of entries in the hash
table with multiple parallel threads is safe on the same common hash table.

### Q1 a

> Q) Modify the insert and retrieve functions so that you do not lose items when running the
program with multiple threads a) You will likely need to use parts of the pthread library; things like `pthread_mutex_t, pthread_mutex_init, pthread_mutex_lock, and/or pthread_mutex_unlock`

In the parallel_mutex.c code, we added mutex to help prevent race conditions
and the loss of entries in the hash table. In the insert() function, we lock mutex before adding an
entry to the table and unlock mutex after modification is done using:

![Mutex](./images/Pthread_Mutex_lock.png)




Run the following code to generate capture the output in a file `timing_out.txt`

```bash
chmod +x timing.sh
./timing.sh                
```


**File: timing_output.txt**
![Keys Lost Run](./images/keys_lost.png)
**As you can see ** the keys lost are actually zero. 

Then do a `grep -v -i "thread" > processed_timing_output.txt` to get the processed output


### Q1 a
> Q:  Once your insert and retrieve functions are correct, compare the time taken to run without
the mutex (original) vs. with the mutex (your changes) for various numbers of threads.
a. In your writeup, show a line plot with the x-axis being the number of threads, the
y-axis being the time taken to complete, and two lines as the series (one for the
original code and one for the mutex-based code.

**Table 1: Insertion Performance Comparison**


| num_threads | insertion original (A) | insertion mutex (B) | Difference (B-A) |
|-------------|------------------------|---------------------|------------------|
| 1 | 0.005110s | 0.007758s | +0.002648s |
| 2 | 0.004284s | 0.014059s | +0.009775s |
| 4 | 0.003564s | 0.021524s | +0.017960s |
| 8 | 0.002563s | 0.024176s | +0.021613s |
| 16 | 0.002439s | 0.023573s | +0.021134s |
| 32 | 0.002243s | 0.023370s | +0.021127s |
| 64 | 0.002771s | 0.022844s | +0.020073s |
| 128 | 0.003493s | 0.025407s | +0.021914s |
| 256 | 0.006025s | 0.025037s | +0.019012s |


---

**Table 2: Insertion Performance Comparison**

| num_threads | retrieval original (A) | retrieval mutex (B) | Difference (B-A) |
|-------------|------------------------|---------------------|------------------|
| 1 | 5.537330s | 5.360113s | -0.177217s |
| 2 | 2.516829s | 2.593512s | +0.076683s |
| 4 | 1.205629s | 1.302376s | +0.096747s |
| 8 | 0.617733s | 0.651315s | +0.033582s |
| 16 | 0.449455s | 0.472166s | +0.022711s |
| 32 | 0.436435s | 0.524325s | +0.087890s |
| 64 | 0.412073s | 0.563330s | +0.151257s |
| 128 | 0.431662s | 0.568757s | +0.137095s |
| 256 | 0.468034s | 0.508867s | +0.040833s |

**Key observation:** The mutex version has modest retrieval overhead (~3-27%), but critically the original version **loses data** under concurrency (e.g., only 54,070/100,000 keys retrieved at t=32) while mutex maintains 100% correctness.


**Insert Timing Plot**
![Insert Timing Plot](./images/insert_timing_plot.svg)

**Retrieval Timing Plot**
![Retrieval Timing Plot](./images/retrieve_timing_plot.svg)



### Q1 b
> In your writeup, estimate the time overhead that your new implementation uses (i.e., how much slow down is required to guarantee correctness using mutexes); explain your estimate

This comes with the expense of spending more time on mutex operations. When you see the two plots that compare insert and retrieve time for the original code vs mutex-based cod - The overhead of adding mutex-base implementation takes an average of about 0.02 seconds more to insert all entries to the hash table than the original code and between 0.2-.18 seconds more to retrieve all entries from the hash table. I derived the number by calculating  the average of the difference between the original and mutex across different numbers of threads `[1,2,4,8,12,16,24,32,48,64,128,256]`.



# SpinLock

### Q2 

> If you were to replace all mutexes with spinlocks, what do you think will happen to the running time? Write a short answer describing what you expect to happen, and why the differences in mutex vs. spinlock implementations lead you to that conclusion (it’s okay if your intuition turns out to be wrong, but start with this answer first).

When replacing all mutexes with spinlocks, we would expect the running time to be longer because while waiting to acquire the lock, a spinlock continuously spins in a loop and consumes the CPU. In our case, inserting an entry item into the hash table requires four operations between locking and unlocking shared variables between all threads.

