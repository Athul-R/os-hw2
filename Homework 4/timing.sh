#!/bin/bash

gcc -pthread parallel_hashtable.c -o parallel_hashtable
gcc -pthread parallel_mutex.c -o parallel_mutex

for t in 1 2 4 8 16 32 64 128 256 ;
     do 
        echo "--- original t=$t ---";
        ./parallel_hashtable $t; 

      echo "--- mutex t=$t ---"; 
      ./parallel_mutex $t; 
    done | tee timing_output.txt