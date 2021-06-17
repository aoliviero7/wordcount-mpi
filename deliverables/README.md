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

> Nunc  fermentum,  arcu  sed  iaculis  ultrices,  enim  arcu  blandit  nibh,bibendum auctor nisi magna ac velit.  
> Phasellus egestas vehicula la-cus nec tincidunt.  Pellentesque diam metus, vulputate eget faucibuseget, maximus at nulla.  
> Nunc volutpat turpis vel leo sagittis, in con-dimentum eros aliquet.  Aliquam finibus, erat id hendrerit tincidunt,
> mi  sapien  hendrerit  mauris,  vehicula  malesuada  turpis  turpis  quisest.    Fusce  sodales  condimentum  enim,  
> et  placerat  metus  tempussed.   Phasellus  lectus  ante,  ullamcorper  at  placerat  ac,  scelerisquesed dolor.  
> Nam egestas nibh eu risus convallis,  et ullamcorper odiofermentum.

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

Whenever a process finishes processing its piece of data, it performs a *MPI_Gather* to tell the master the size of its local array. The MPI type is created for the Word structure and then, via *MPI_Gatherv*, all the data of the slaves are collected from the master. The same words received from different processes are grouped, and finally the resulting array is sorted in descending order of the word frequencies and a csv file is created for the output. The file is called ***ResultsParallel.csv***.

## Parallel Execution

Inside the ***wordcount-mpi*** directory run the following command:

    mpirun -np NUMBER_OF_PROCESSORS wordcount 

Add *--allow-run-as-root* and *--oversubscribe*, if necessary:

    mpirun -np --allow-run-as-root --oversubscribe NUMBER_OF_PROCESSORS wordcount 

In this way you can run the program and count word of the files into ***wordcount-mpi/txt*** folder; if you would like to try different input .txt files, you can replace the ones already present.

## Sequential Solution

Inside the ***wordcount-mpi*** directory there is a file called ***sequential.c*** that allows you to run the same algorithm implemented in the described solution, but without all those features that generate overhead such as: the byte count of each file, the calculation of which portion of the files to analyze, the grouping of information by the master and in general all communication. The file that will be generated is called ***ResultSequential.csv***
This is to be able to compare the results obtained more correctly.
Inside the ***wordcount-mpi*** directory run the following command:

    sequential 

## Results Correctness

To analyze the results produced by the two solutions, parallel and sequential, both were performed on a wide range of inputs of different dimensions divided into one or more files. For a simple understanding, a short input file with relative output is shown.
**Input:**

> Nunc  fermentum,  arcu  sed  iaculis  ultrices,  enim  arcu  blandit  nibh,bibendum auctor nisi magna ac velit.  
> Phasellus egestas vehicula la-cus nec tincidunt.  Pellentesque diam metus, vulputate eget faucibuseget, maximus at nulla.  
> Nunc volutpat turpis vel leo sagittis, in con-dimentum eros aliquet.  Aliquam finibus, erat id hendrerit tincidunt,
> mi  sapien  hendrerit  mauris,  vehicula  malesuada  turpis  turpis  quisest.    Fusce  sodales  condimentum  enim,  
> et  placerat  metus  tempussed.   Phasellus  lectus  ante,  ullamcorper  at  placerat  ac,  scelerisquesed dolor.  
> Nam egestas nibh eu risus convallis,  et ullamcorper odiofermentum.

**Output:**

    Word, Count
    turpis, 3
    nunc, 2
    arcu, 2
    enim, 2
    nibh, 2
    ac, 2
    phasellus, 2
    egestas, 2
    vehicula, 2
    tincidunt, 2
    metus, 2
    at, 2
    hendrerit, 2
    et, 2
    placerat, 2
    ullamcorper, 2
    fermentum, 1
    sed, 1
    iaculis, 1
    ...

## Benchmark

After analyzing the correctness of the proposed solution, we moved on to the testing and benchmarking phase on clusters. **Amazon Web Services** (**AWS**) was used which provides cloud computing services on an on-demand platform. 

Two main metrics for performance evaluation were used for this phase, such as:
***Speedup*** and ***Efficiency***.

The *Speedup* is used to indicate the increase in performance between sequential execution and parallel execution, with the same input.

The *Efficiency* is a normalization of the *Speedup* and serves to indicate how close the sequential and parallel execution times are, always with the same input.

After defining the architecture used, the results will be displayed in terms of: 
- Strong Scalability; 
- Weak Scalability.

### Architecture

The benchmarks were performed on an AWS cluster of 8 t2.small instances, each with:
- 1 vCPU
- 2 GiB of RAM
- Ubuntu Linux 18.04 LTS Server Edition - ami-0747bdcabd34c712a
- 8 GiB EBS storage

### Strong Scalability

In Strong Scalability, the execution time of the program is calculated with constant input size, but with the variation of the number of processors.
Let Tk be the execution time of the program on k processors and N the number of processors, are also calculated:
- *Strong Scalability Speedup* = T1 / TN
- *Strong Scalability Efficiency* = T1 / (N ∗ TN)

The input, which will remain constant for all executions, consists of 6.38 MB of data divided into the following files:
- *25k_1.txt* (175 KB): a file of about 25000 random words in English with allowed repetitions;
- *The Hitchhikers Guide to the Galaxy.txt* (280 KB): a book by Douglas Adams;
- *HP - The Chamber of Secrets.txt* (539 KB): the second chapter of the Harry Potter saga;
- *HP - The Goblet of Fire.txt* (1.16 MB): the fourth chapter of the Harry Potter saga;
- *Bible_KJV.txt* (4.24 MB): The King James Bible.

The results obtained are reported below.


The graph show how execution times decrease as processes increase. The execution time falls inversely proportional to the increase of the processors involved; this up to 4 processes, after which it decreases more linearly. This behavior can be highlighted more with the charts relating to Speedup and Efficiency.


As previously mentioned, both the Speedup and the Efficiency are excellent up to 4 processors used, after which the performances improve in any case but in a less evident way, reaching in the worst case an efficiency of 63% and a Speedup value of 5.07 / 8.

### Weak Scalability

In Weak Scalability, the execution time of the program is calculated with the size of the input that grows proportionally to the number of nodes in the cluster.

Let Tk be the execution time of the program on k processes, it is also calculated:

*Weak Scalability Efficiency* = T1 / TN

The input will consist of a file of this type for each process involved:
- *25k_N.txt* (175 KB): a file of about 25000 random words in English with allowed repetitions.

| Processes 	|  1  	|  2  	|  3  	|   4  	|   5  	|   6  	|   7  	|   8  	|
|:---------:	|:----:	|:----:	|:----:	|:----:	|:----:	|:----:	|:----:	|:----:	|
|   Words   	| 25k 	| 50k 	| 75k 	| 100k 	| 125k 	| 150k 	| 175k 	| 200k 	|

As you can see from the graph, the execution time grows slightly, but steadily as the processors increase; this is due to the cost of communication between the various cluster nodes. In fact, it can be noted that the greatest time difference is obtained by passing from 1 (therefore no communication between different processors) to 2 processors involved.


From this other graph, however, it can be seen that, due to the times previously seen, also the efficiency has a similar behavior: it decreases slightly in a constant way as the number of processors increases. The reasons are the same, and here too there is a greater difference, of about 19%, between the use of 1 and 2 processors.

##Conclusions
The Word Count problem has been presented which consists in determining the word frequency of the input files. Then a parallel solution to the problem was provided using MPI, analyzing the approach used and the implementation code. Finally, the results in terms of Strong and Weak Scalability were analyzed on a cluster of 8 nodes implemented on AWS. The solution has shown excellent results with the use of up to 4 processors; subsequently, from 4 to 8 processors, the results can still be considered discrete.
