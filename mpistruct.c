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

#define NELEM 2

int main(int argc, char *argv[])  {
    int numtasks=0, rank, source=0, dest, tag=1, i;

    typedef struct{
        char parola[100];
        int count;
    }Word;
    
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
    blockcounts[1] = 1;

    // define structured type and commit it
    MPI_Type_create_struct(2, blockcounts, offsets, oldtypes, &Wordtype);
    MPI_Type_commit(&Wordtype);
    int sizeToSend = NELEM*(rank+1);
    Word     Words[sizeToSend];

    for (i=0; i<sizeToSend; i++) {
        strcpy(Words[i].parola , "pippo");
        //printf("hi %d\n",i);
        Words[i].count = i * 10;
    }
    
    int size;
    MPI_Type_size(Wordtype, &size);
    
    int displacements[numtasks], count[numtasks], tot=0;																//variabili richieste per l'invio
	if(rank==0)
		for(int i=0; i<numtasks; i++){															//il master calcola indici e dimensioni
			displacements[i] = (i==0) ? 0 : displacements[i-1] + (NELEM*i);
            count[i] = (i+1) * NELEM;
			printf("displacements[%d]: %d\n",i,displacements[i]);
            printf("count[%d]: %d\n",i,count[i]);
            tot+=count[i];
            //tot+=NELEM;
		}
    printf("tot: %d\n",tot);
    Word recv[tot];
    MPI_Gatherv(Words, sizeToSend, Wordtype, recv, count, displacements, Wordtype, 0, MPI_COMM_WORLD);
    //MPI_Gather(Words, NELEM, Wordtype, recv, NELEM, Wordtype, 0, MPI_COMM_WORLD);
    for (i=0; i<tot; i++){
        printf("Parola: %s - counts: %d\n",recv[i].parola,recv[i].count);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    
    MPI_Type_free(&parolatype);
    MPI_Type_free(&Wordtype);
    MPI_Finalize();
}