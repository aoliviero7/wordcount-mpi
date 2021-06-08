# A paraller WordCount using mpi in C

The word count is the number of words in a document or passage of text. Word counting may be needed when a text is required to stay within specific numbers of words.
I made this simple implementation of word count in mpi during the "Comptitive, Parallel and Cloud Programming" course (PCPC - 0522500102) at the University of Salerno.

## Problem Statement

We will be doing a version of map-reduce using MPI to perform word counting over a large number of files:
1. the MASTER node reads the file list (or a directory), which will contain the names of all the files that are to be counted. Note that only 1 of your processes should read the files list. Then each of the processes should receive their portion of the file from the MASTER process. Once a process has received its list of files to process, it should then read in each of the files and perform a word counting, keeping track of the frequency each word found in the files occurs. We will call the histogram produced the local histogram. This is similar to the map stage or map-reduce.
2. the second phase is combining the frequencies of words across processes. For example, the word 'cat' might be counted in multiple processes, and we need to add up all these occurrences. This is similar to the reduced stage of map-reduce.
3. the last phase is to have each of the processes send their local histograms to the master process. The MASTER process just needs to gather up all this information. Note that there will be duplicate words between processes. The master should create a CSV formatted file with the words and frequencies ordered.

## Solution Approach

Words entered in files can be up to 100 characters long. The approach used allows you to read words from a file, without constraints on how it must be formatted: for example, the words it contains do not have to be separated by the '\n' character, or by any other "special character"; simply one or more blank spaces, punctuation marks and so on.
In this way, it is also possible to potentially analyze books, or text files of this type:

> This is the law of jealousies, when a wife goeth aside to another
> instead of her husband, and is defiled; or when the spirit of
> jealousy cometh upon him, and he be jealous over his wife, and shall
> set the woman before the LORD, and the priest shall execute upon her
> all this law.

My solution consists of:
1. Calculate the sum of the bytes of all the files to be analyzed.
2. Divide (equally) the bytes of the files to be read for each process.
3. Each process calculates its local histogram and sends it to the master.
4. The master process brings together the information received.

**Step. 1**

In *fileScan()* function are scanned the files in the folder defined, and are calculated the total byte to be analyzed:
- the directory is opened;
- using *fseek()* you go to the end of the file;
- using *ftell()* you take the position of the pointer (file size).

**Step. 2**

In *chunkAndCount()* function are calculated byte to be analyzed for each process. Division by byte was chosen as the solution because dividing by words would generate more overhead for the initial count of them. Each process calculates this data to avoid unnecessary communication. Both how many bytes each process must analyze, and from which byte of which file it has to start reading and from which byte of which file it has to end are calculated.
For example:
> The process 3 must analyze and works 1000 bytes from byte 400 of file 5 up to 150 bytes of the file 7
Clearly if the division of the total bytes to be analyzed on the number of processes is not perfect, the first "remainder" processors analyze an extra byte.

**Step. 3**

In *wordCount()* function each process analyzes its portion of the files and returns an array of the Word struct, consisting of the word and the number of its occurrences. The basic functioning of the algorithm is very simple: every time a process reads a word from the file it checks if it is present in its local array, if yes it increments its counter, if not it must add an extra block to the array, insert the word and initialize its counter to 1.

Each word is read from the file through the *getWord()* function, which extracts the word by reading character by character, avoiding reading non-alphanumeric characters such as punctuation marks or whitespace; the word is returned with all lowercase characters to avoid mismatching problems between identical words. Using this technique, words separated by a punctuation mark, such as a hyphen, are treated as two different words.

Since the portions to be analyzed have been divided according to bytes and not words, it is possible that a process ends in the middle of a word and consequently another starts in the middle of that word. This situation has been handled in such a way that the process that should end in the middle of the word reads it in its entirety and places it in its array, while the other process checks if it starts in the middle of a word, if yes it ignores it and goes to the next word, otherwise it processes it.

**Step. 4**

Whenever a process finishes processing its piece of data, it performs a *MPI_Gather* to tell the master the size of its local array. The MPI type is created for the Word structure and then, via *MPI_Gatherv*, all the data of the slaves are collected from the master. The same words received from different processes are grouped, and finally the resulting array is sorted in descending order of the word frequencies and a csv file is created for the output.


