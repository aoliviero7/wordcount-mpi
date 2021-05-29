#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>

#define FOLDER "txt/try"

typedef struct{
	char name[100];
	int size;
}fileStats;

void wordCount(int inizio, int fine, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank);


int main(int argc, char** argv) {
	MPI_Init(NULL, NULL);
    int myrank,p;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	fileStats *myFiles;
	int *fileByte;
	int countFiles = 0, totalByte = 0;
	
	directory = opendir(FOLDER);
	if(directory){
		while(Dirent=readdir(directory))								//conto i file
			countFiles++;
		seekdir(directory, 0);
		myFiles = (fileStats*) malloc(sizeof(fileStats) * countFiles);	//alloco la struttura con il numero dei file
		countFiles = 0;
		while(Dirent=readdir(directory)){
			char filepath[100] = FOLDER;
			strcat(filepath, "/");
			strcat(filepath, Dirent->d_name);
			//printf("Filepath: %s\n",filepath);
			if(Dirent->d_type==8){										//regoular file
				file = fopen(filepath, "r");							//accedo in lettura
				strcpy(myFiles[countFiles].name,Dirent->d_name);		//salvo il nome
				//printf("Filename: %s\n",Dirent->d_name);
				if(file){									
					fseek(file, 0L, SEEK_END);							//conto i byte dei file posizionandomi alla fine
					myFiles[countFiles].size = ftell(file);				//salvo la dimensione
					totalByte += myFiles[countFiles++].size;			//aggiorno il contatore totale dei byte
				}
				fclose(file);
			}
		}
		if(myrank==0){
			for(int i=0; i<countFiles; i++)
				printf("Nome file %d: %s, Byte %d\n", i, myFiles[i].name, myFiles[i].size);
		}
	}
	else
		printf("Directory non leggibile\n");
	closedir(directory);
	
	/*
		Calcolo la porzione di byte che ogni processo deve elaborare
		Es: il processo 3 lavora dal byte 400 del file 5 fino al byte 150 del file 7
	*/
	int resto = totalByte % p;
	int byteProcess = totalByte / p;																		
	int inizio = (myrank<resto) ? (byteProcess + 1) * myrank : resto + (byteProcess * myrank);
	int fine = (myrank<resto) ? (byteProcess + 1) * (myrank + 1) : resto + (byteProcess * (myrank + 1));
	printf("Byte totali: %d, Byte per process: %d, resto: %d\n",totalByte,byteProcess,resto);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("Processore: %d, Byte inizio: %d - Byte fine: %d\n",myrank,inizio,fine);
	int inizioFile = 0, fineFile = 0;
	for(int i = 0; i<countFiles; i++){
		if(inizio<=myFiles[i].size){
			inizioFile = i;
			break;
		}else
			inizio -= myFiles[i].size;
	}
	for(int i = 0; i<countFiles; i++){
		if(fine<=myFiles[i].size){
			fineFile = i;
			break;
		}else
			fine -= myFiles[i].size;
	}
	printf("Processore: %d, File inizio: %d, Byte: %d - File fine: %d, Byte: %d\n",myrank,inizioFile,inizio,fineFile,fine);
	
	wordCount(inizio, fine, inizioFile, fineFile, resto, byteProcess, myFiles, myrank);
	
	free(myFiles);
	MPI_Finalize();
}

void wordCount(int inizio, int fine, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank){
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	int size = 0, countByte = 0;
	char **parole;
	int *count;
	count = (int*) malloc(sizeof(int));
	parole = (char**) malloc(sizeof(char*));
	byteProcess = (myrank<resto) ? byteProcess + 1 : byteProcess;
	
	directory = opendir(FOLDER);
	if(directory){
		for(int iFile = inizioFile; iFile <= fineFile; iFile++){
			char filepath[100] = FOLDER;
			strcat(filepath, "/");
			strcat(filepath, myFiles[iFile].name);
			file = fopen(filepath, "r");
			if(file){
				char str[100];
				if(iFile == inizioFile)
					fseek(file, inizio, SEEK_SET);				//mi posiziono al primo byte di lettura se è il primo file da leggere
										
				while(!feof(file)){								//scansiono il file
					if(countByte<byteProcess){					//non devo leggere di più di quanto mi tocca
						int prima = ftell(file);
						fscanf(file," %s ", str);				//prendo una parola
						printf("Stringa letta: %s \n",str);
						if(iFile == inizioFile && prima == inizio && myrank != 0){	//se sto leggendo la prima parola del primo file
							fseek(file, inizio-1, SEEK_SET);						//mi sposto di un byte(carattere) indietro
							char str2[100];											
							fscanf(file," %s ", str2);								//leggo la parola da un carattere indietro
							if(strcmp(str,str2) && strlen(str2)!=1)					//se la parola è la stessa oppure è di lunghezza 1 
								strcpy(str,str2);									//(in pratica ci si trova a cavallo tra una parola e l'altra), allora la posso leggere
						}
						
						int dopo = ftell(file);
						countByte+=(dopo-prima);				//aggiorno i byte letti
							
						int flag = 0, i = 0;
						for(i=0; i<size; i++){					//cerco nell'array se è già presente la parola
							if(!strcmp(str, parole[i])){		//se le parole sono uguali
								flag = 1;						//setto il flag a true
								break;							
							}
						}
						if(flag)								//flag = 1
							count[i]++;							//la parola esiste già e incremento il suo contatore
						else{									//flag = 0
							size++;								//incremento la size e aggiungo gli elementi agli array
							parole = (char**) realloc(parole, size * sizeof(char*));
							parole[size-1] = (char*) malloc(100 * sizeof(char));
							count = (int*) realloc(count, size * sizeof(int));
							strcpy(parole[size-1], str);
							count[size-1] = 1;
						}
					}else
						break;
				}
			}
			fclose(file);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		sleep(myrank);
		printf("Processo: %d \n",myrank);
		for(int i=0; i<size; i++){
			printf("Parola: %s - counts: %d\n",parole[i],count[i]);
		}
		free(parole);
		free(count);
	}
}

