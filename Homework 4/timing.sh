#!/bin/bash

gcc -pthread parallel_hashtable.c -o parallel_hashtable
gcc -pthread parallel_mutex.c -o parallel_mutex
gcc -pthread parallel_spin.c -o parallel_spin  # Comment this for the first run.

for t in 1 2 4 8 16 32 64 128 256 ;
     do 
      echo "--- original t=$t ---";
      ./parallel_hashtable $t; 

      echo "--- mutex t=$t ---"; 
      ./parallel_mutex $t; 

      echo "--- spinlock t=$t ---"; 
      ./parallel_spin $t; 
    done | tee timing_output_with_spin.txt  # Rename it to timing_output.txt for the first run.