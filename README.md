# A paraller WordCount using mpi in C

The word count is the number of words in a document or passage of text. Word counting may be needed when a text is required to stay within specific numbers of words.
I made this simple implementation of word count in mpi during the "Comptitive, Parallel and Cloud Programming" course (PCPC - 0522500102) at the University of Salerno.

## Problem Statement

We will be doing a version of map-reduce using MPI to perform word counting over a large number of files:
1. the MASTER node reads the file list (or a directory), which will contain the names of all the files that are to be counted. Note that only 1 of your processes should read the files list. Then each of the processes should receive their portion of the file from the MASTER process. Once a process has received its list of files to process, it should then read in each of the files and perform a word counting, keeping track of the frequency each word found in the files occurs. We will call the histogram produced the local histogram. This is similar to the map stage or map-reduce.
2. the second phase is combining the frequencies of words across processes. For example, the word 'cat' might be counted in multiple processes, and we need to add up all these occurrences. This is similar to the reduced stage of map-reduce.
3. the last phase is to have each of the processes send their local histograms to the master process. The MASTER process just needs to gather up all this information. Note that there will be duplicate words between processes. The master should create a CSV formatted file with the words and frequencies ordered.

## Approach

My solution consists of:
1. Calculate the sum of the bytes of all the files to be analyzed.
2. Divide (equally) the bytes of the files to be read for each process.
3. Each process calculates its local histogram and sends it to the master.
4. The master process brings together the information received.
