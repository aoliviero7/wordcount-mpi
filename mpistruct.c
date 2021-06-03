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

#define NELEM 12500

int main(int argc, char *argv[])  {
    int numtasks=0, rank, source=0, dest, tag=1, i;

    typedef struct{
        char parola[100];
        int count;
    }Word;
    Word     Words[NELEM];
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

    printf("hi \n");
    for (i=0; i<NELEM; i++) {
        strcpy(Words[i].parola , "pippo");
        //printf("hi %d\n",i);
        Words[i].count = i * 10;
    }
    
    printf("rank %d ora invio\n",rank);
    
    
    int size;
    MPI_Type_size(Wordtype, &size);
    //printf("size %d\n",size);
    char message[NELEM * size]; 
    char p[numtasks * NELEM * size];
    //Word recv[numtasks * NELEM];
    Word recv[numtasks][NELEM];
    /*p=calloc(numtasks, sizeof(char*));
    if(p==NULL)
        printf("null 1\n");
    for (i=0; i<numtasks; i++) {
        p[i]= calloc(NELEM, size);
        if(p[i]==NULL)
            printf("null 2\n");
    }*/

    
    int position = 0;
    for (i=0; i<NELEM; i++) 
        MPI_Pack(&Words[i], 1, Wordtype, message, NELEM *size, &position, MPI_COMM_WORLD);

    MPI_Gather(message, NELEM*size, MPI_PACKED, p, NELEM*size, MPI_PACKED, 0, MPI_COMM_WORLD);
    
    position = 0;
    for (i=0; i<numtasks; i++){
        position = 0;
        for (int j=0; j<NELEM; j++) 
            MPI_Unpack(p, NELEM * size, &position, &recv[i][j], 1, Wordtype, MPI_COMM_WORLD);
    }

    //MPI_Gather(Words, NELEM, Wordtype, recv, NELEM, Wordtype, 0, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    
    if(rank==0)
        for (i=0; i<numtasks; i++) 
            for (int j=0; j<NELEM; j++) 
                printf("rank= %d   %s %d\n",i , recv[i][j].parola, recv[i][j].count);
    //for (i=0; i<numtasks; i++) 
    //    free(p[i]);
    
    //free(p);
    MPI_Type_free(&parolatype);
    MPI_Type_free(&Wordtype);
    MPI_Finalize();
}