#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#define NELEM 100000
#define MYBUF 20000

int main(int argc, char *argv[])  {
    int numtasks, rank, source=0, dest, tag=1, i;

    typedef struct{
        char parola[100];
        int count;
    }Word;
    Word     p[NELEM], Words[NELEM];
    MPI_Datatype Wordtype, oldtypes[2],parolatype;   // required variables
    int          blockcounts[2];

    // MPI_Aint type used to be consistent with syntax of
    // MPI_Type_extent routine
    MPI_Aint    offsets[2], lb, extent;

    MPI_Status stat;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

    MPI_Type_contiguous(100, MPI_CHAR, &parolatype);
    MPI_Type_commit(&parolatype);

    // setup description of the 100 MPI_CHAR (parolatype) fields parola
    offsets[0] = 0;
    oldtypes[0] = parolatype;
    blockcounts[0] = 1;

    // setup description of the 1 MPI_INT fields count
    // need to first figure offset by getting size of parolatype
    MPI_Type_get_extent(parolatype, &lb, &extent);
    //printf("sizeof wchar %ld\n",sizeof(char));
    //printf("lb: %ld extent:%ld\n",lb,extent);
    offsets[1] = 1 * extent;
    oldtypes[1] = MPI_INT;
    blockcounts[1] = 2;

    // define structured type and commit it
    MPI_Type_create_struct(2, blockcounts, offsets, oldtypes, &Wordtype);
    MPI_Type_commit(&Wordtype);

    int size;
    MPI_Type_size(Wordtype, &size);
    //printf("size %d\n",size);
    char message[NELEM * size]; 

    // task 0 initializes the Word array and then sends it to each task
    if (rank == 0) {
        printf("hi \n");
        for (i=0; i<NELEM; i++) {
            strcpy(Words[i].parola , "pippo");
            printf("hi %d\n",i);
            Words[i].count = i * 10;
            }
        /*
        for (i=0; i<numtasks; i++) {
            printf("rank %d ora invio\n",rank);
            MPI_Send(Words, NELEM, Wordtype, i, tag, MPI_COMM_WORLD);
        }*/

        int position = 0;
        printf("rank %d ora packo\n",rank);
        /* now let's pack all those values into a single message */
        for (i=0; i<NELEM; i++) 
            MPI_Pack(&Words[i], 1, Wordtype, message, NELEM * size, &position, MPI_COMM_WORLD);
        printf("rank %d ora invio\n",rank);
        MPI_Send(message, NELEM*size, MPI_PACKED, 1, 1, MPI_COMM_WORLD);
    }else{
        printf("rank %d ora ricevo\n",rank);
        MPI_Recv(message, NELEM*size, MPI_PACKED, 0, 1, MPI_COMM_WORLD, NULL);
        printf("rank %d ora unpacko\n",rank);
        int position = 0;
        for (i=0; i<NELEM; i++) 
            MPI_Unpack(message, NELEM * size, &position, &p[i], 1, Wordtype, MPI_COMM_WORLD);

        /*
        // all tasks receive Wordtype data
        MPI_Recv(p, NELEM, Wordtype, source, tag, MPI_COMM_WORLD, &stat);
        */

        for (i=0; i<NELEM; i++) 
            printf("rank= %d   %s %d\n", rank, p[i].parola, p[i].count);
    }
    // free datatype when done using it
    MPI_Type_free(&parolatype);
    MPI_Type_free(&Wordtype);
    MPI_Finalize();
}