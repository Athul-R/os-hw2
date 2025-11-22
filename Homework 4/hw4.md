# **Homework 4:  Concurrency**

---

[**Background	2**](#background)

[**Intro \[00 Points\]	2**](#intro-[00-points])

[**Mutex \[30 Points\]	3**](#mutex-[30-points])

[**Spinlock \[30 Points\]	4**](#spinlock-[30-points])

[**Mutex, Retrieve Parallelization \[20 Points\]	4**](#mutex,-retrieve-parallelization-[20-points])

[**Mutex, Insert Parallelization \[20 Points\]	5**](#mutex,-insert-parallelization-[20-points])

[**Submission	5**](#submission)

[**Academic Honesty	5**](#academic-honesty)

**General Notes**  
Read the assignment requirements carefully, especially what to include in your final  
submission for each part. Refer to the submission instructions on the bottom of this document.

# Background {#background}

---

This assignment will use the pthreads (Posix Threads) specification and APIs. This assignment will NOT use xv6. You will need to use a machine (or VM) that has multiple cores available, as well as the GCC compiler.

# Intro \[00 Points\] {#intro-[00-points]}

---

In this assignment, you will take a non-thread-safe version of a hash table and modify it so that it correctly supports insertions and retrievals from multiple threads.

Start by downloading the file [parallel\_hashtable.c](https://drive.google.com/file/d/1KvsolZjR6_WKEbAJ9_O3EsiJGSCXREgb/view?usp=drive_link) (you can also find it [here](https://github.com/tsoestudents/operating-systems-fall25/tree/main/homework04)) and compile it with the following command:

| $ gcc \-pthread parallel\_hashtable.c \-o parallel\_hashtable |
| :---- |

Run this program with 1 thread:

| $ ./parallel\_hashtable 1 |
| :---- |

And you should see an output similar to the following:

| \[main\] Inserted 100000 keys in 0.006545 seconds\[thread 0\] 0 keys lost\!\[main\] Retrieved 100000/100000 keys in 4.028568 seconds |
| :---- |

With one thread the program is correct, but try it with more than one thread

| ./parallel\_hashtable 8\[main\] Inserted 100000 keys in 0.002476 seconds\[thread 7\] 4304 keys lost\!\[thread 6\] 4464 keys lost\!\[thread 2\] 4273 keys lost\!\[thread 1\] 3864 keys lost\!\[thread 4\] 4085 keys lost\!\[thread 5\] 4391 keys lost\!\[thread 3\] 4554 keys lost\!\[thread 0\] 4431 keys lost\!\[main\] Retrieved 65634/100000 keys in 0.792488 seconds |
| :---- |

In general, the program gets “faster” in (terms of total execution time) up to a certain number of threads. However, items that are inserted into the hashtable are getting “lost.”

# Mutex \[30 Points\] {#mutex-[30-points]}

---

What circumstances cause an entry to get lost? Analyze the initial code and write a short answer to describe what it means for an entry to be “lost,” and which parts of the program are causing this unintended behavior when run with multiple threads.

Next, copy the initial code to a new file parallel\_mutex.c; we’re going to start first with using mutex(es) to correct the problems you’ve identified above. 

1. Modify the insert and retrieve functions so that you do not lose items when running the program with multiple threads  
   1. You will likely need to use parts of the pthread library; things like pthread\_mutex\_t, pthread\_mutex\_init, pthread\_mutex\_lock, and/or pthread\_mutex\_unlock  
2. Once your insert and retrieve functions are correct, compare the time taken to run without the mutex (original) vs. with the mutex (your changes) for various numbers of threads.  
   1. In your writeup, show a line plot with the x-axis being the number of threads, the y-axis being the time taken to complete, and two lines as the series (one for the original code and one for the mutex-based code)  
   2. In your writeup, estimate the time overhead that your new implementation uses (i.e., how much slow down is required to guarantee correctness using mutexes); explain your estimate

*Include: working code for parallel\_mutex.c; In PDF: short answer for analysis of prior code’s unintended behaviors, a graph of prior running time vs. updated running time, an estimate of the timing overhead, and an explanation of how you came up with that estimate*

# Spinlock \[30 Points\] {#spinlock-[30-points]}

---

If you were to replace all mutexes with spinlocks, what do you think will happen to the running time? Write a short answer describing what you expect to happen, and why the differences in mutex vs. spinlock implementations lead you to that conclusion (it’s okay if your intuition turns out to be wrong, but start with this answer first).

Now, copy your parallel\_mutex.c code to parallel\_spin.c; we’re going to test your hypothesis by replacing all mutexes with spinlocks (things like pthread\_mutex\_t become pthread\_spinlock\_t, and pthread\_mutex\_lock becomes pthread\_spin\_lock). 

Once these modifications have been made, show another line plot with the x-axis being the number of threads, the y-axis being the time taken to complete, and three lines as the series (one for original/unsafe, one for mutex-based, and one for spinlock-based.)  
Lastly, estimate the time overhead that your spinlock implementation uses and explain your estimate.

*Include: working code for parallel\_spin.c; In PDF: short answer for what you expect to happen when replacing mutex(es) with spinlock(s), explanation for why you have that hypothesis (based on the differences in how a mutex operates vs. a spinlock), a graph of prior running time vs. mutex running time vs. spinlock running time, an estimate of the timing overhead of spinlocks, and an explanation of how you came up with that estimate*

# Mutex, Retrieve Parallelization \[20 Points\] {#mutex,-retrieve-parallelization-[20-points]}

---

Let’s revisit your mutex-based code. When we retrieve an item from the hash table, do we need a lock? Write a short answer and explain why or why not.

Copy your parallel\_mutex.c code to parallel\_mutex\_opt.c. In this new program, update your code so that multiple retrieve operations can run in parallel.

Once these modifications have been made, explain what you’ve changed.

*Include: (together with part 5\) working code for parallel\_mutex\_opt.c; In PDF: short answer for why we do or don’t need a lock for retrieval, and an explanation of what you’ve changed to allow retrieval to be parallelizable*

# Mutex, Insert Parallelization \[20 Points\] {#mutex,-insert-parallelization-[20-points]}

---

Last, let’s consider insertions. Describe a situation in which multiple insertions could happen safely (hint: what’s a bucket?).

Update your parallel\_mutex\_opt.c to handle this scenario, and explain what you’ve changed.

*Include: (together with part 4\) working code for parallel\_mutex\_opt.c; In PDF: short answer for when insertions could be safely parallelized, and an explanation of what you’ve changed in order to allow insertion to be parallelizable*

# Submission {#submission}

---

1. Follow this repository structure  
   [https://github.com/tsoestudents/operating-systems-fall25/tree/main/homework04](https://github.com/tsoestudents/operating-systems-fall25/tree/main/homework04)

2. You need to do this inside the same repository that you used for Homework-2. Essentially, refactor the code you wrote for earlier HW inside the first folder, and then write all the code for Homework04 inside a different folder. The structure should be the same as the aforementioned github repository.

3. In the [README.md](http://README.md) file, put in all the answers that were asked. If you’re making any assumptions, add a section and write those there. You can create a pdf file and add your answers there if you’re not comfortable writing on the README. However, make sure that you mention it on the README file. 

4. Put your partner name in the .txt file, which is also a part of the repository.

   **Submit the Github repository link on Brightspace**   
   **(We won’t accept any new requests for adding us as collaborators)**

# Academic Honesty {#academic-honesty}

---

Aside from the narrow exception for collaboration on homework, all work submitted in this course must be your own. Cheating and plagiarism will not be tolerated. If you have any  
questions about a specific case, please ask the Prof/TAs. We will be checking for this\!

NYU Tandon’s Policy on Academic Misconduct:  
[http://engineering.nyu.edu/academics/code-of- conduct/academic-misconduct](http://engineering.nyu.edu/academics/code-of-)