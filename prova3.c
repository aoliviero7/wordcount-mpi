#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>

#define FOLDER "txt/try"

typedef struct{
	char name[100];
	int size;
}fileStats;

typedef struct{
	char parola[100];
	int count;
}Word;

void wordCount(int inizio, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank);
void chunkAndCount(int myrank, int totalByte, fileStats* myFiles, int countFiles, int p);
int getWord(FILE *file, char* str);


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
	
	chunkAndCount(myrank, totalByte, myFiles, countFiles, p);
	
	
	free(myFiles);
	MPI_Finalize();
}


/*
	Qui calcolo la porzione di byte che ogni processo deve elaborare
	Es: il processo 3 lavora dal byte 400 del file 5 fino al byte 150 del file 7
	Dopodiché viene chiamato il metodo wordCount
*/
void chunkAndCount(int myrank, int totalByte, fileStats* myFiles, int countFiles, int p){
	int resto = totalByte % p;
	int byteProcess = totalByte / p;																		
	int inizio = (myrank<resto) ? (byteProcess + 1) * myrank : resto + (byteProcess * myrank);				//da che byte parto (riferito ai byte totali da elaborare)
	int fine = (myrank<resto) ? (byteProcess + 1) * (myrank + 1) : resto + (byteProcess * (myrank + 1));	//a che byte arrivo (riferito ai byte totali da elaborare)
	printf("Byte totali: %d, Byte per process: %d, resto: %d\n",totalByte,byteProcess,resto);
	MPI_Barrier(MPI_COMM_WORLD);
	printf("Processore: %d, Byte inizio totali: %d - Byte fine totali: %d\n",myrank,inizio,fine);
	int inizioFile = 0, fineFile = 0;																		//da che file parto a che file arrivo
	for(int i = 0; i<countFiles; i++){
		if(inizio<=myFiles[i].size){
			inizioFile = i;
			break;
		}else
			inizio -= myFiles[i].size;																		//da che byte parto (riferito al file, es: byte 400 del file 5)
	}
	for(int i = 0; i<countFiles; i++){
		if(fine<=myFiles[i].size){
			fineFile = i;
			break;
		}else
			fine -= myFiles[i].size;																		//a che byte arrivo (riferito al file, es: byte 150 del file 7)
	}
	printf("Processore: %d, File inizio: %d, Byte: %d - File fine: %d, Byte: %d\n",myrank,inizioFile,inizio,fineFile,fine);
	
	wordCount(inizio, inizioFile, fineFile, resto, byteProcess, myFiles, myrank);
}


/*
	Qui ogni processo si calcola la frequenza delle parole delle porzioni dei file che gli spettano
*/
void wordCount(int inizio, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank){
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	int size = 0, countByte = 0;
	Word* words;
	words = (Word*) malloc(sizeof(Word));
	byteProcess = (myrank<resto) ? byteProcess + 1 : byteProcess;
	printf("Processore: %d, Devo leggere: %d Byte\n",myrank,byteProcess);
	
	directory = opendir(FOLDER);
	if(directory){
		for(int iFile = inizioFile; iFile <= fineFile; iFile++){
			char filepath[100] = FOLDER;
			strcat(filepath, "/");
			strcat(filepath, myFiles[iFile].name);
			file = fopen(filepath, "r");
			if(file){
				if(iFile == inizioFile)
					fseek(file, inizio, SEEK_SET);				//mi posiziono al primo byte di lettura se è il primo file da leggere										
				while(!feof(file)){								//scansiono il file
					if(countByte<byteProcess){					//non devo leggere di più di quanto mi tocca
						char str[100];
						int prima = ftell(file);
						getWord(file, str);											//prendo una parola
						int dopo = ftell(file);
						printf("Stringa letta: %s \n",str);
						int salto = 0;												//flag che mi indica se la parola la devo saltare	
						if(iFile == inizioFile && prima == inizio && inizio!=0){	//se sto leggendo la mia prima parola (prima==inizio) del primo file che mi è stato assegnato (iFile==inizioFile),
																					//e non mi trovo all'inizio del file (inizio!=0) (NB: rank 0 non lo farà mai dato che il suo inizio=0)
							fseek(file, inizio-1, SEEK_SET);						//mi sposto di un byte(carattere) indietro
							char c;
							c = fgetc(file);										//leggo il carattere precedente a quello che mi spetta realmente	
							if(c!=' ')												//se il carattere precedente non è uno spazio (quindi fa parte di una parola)
								salto = 1;											//devo saltare la parola poiché è già stata letta dal processo precedente, altrimenti devo leggerla	
							fseek(file, dopo, SEEK_SET);							//mi riposiziono in modo giusto	
						}															
						countByte+=(dopo-prima);				//aggiorno i byte analizzati
						if(salto)								//se non devo leggere la parola
							continue;							//vado alla prossima iterazione perché la parola è già stata analizzata
						
						
							
						int flag = 0, i = 0;
						for(i=0; i<size; i++){					//cerco nell'array se è già presente la parola
							if(!strcmp(str, words[i].parola)){	//se le parole sono uguali
								flag = 1;						//setto il flag a true
								break;							
							}
						}
						if(flag){								//flag = 1
							words[i].count++;					//la parola esiste già e incremento il suo contatore
						}
						else{									//flag = 0
							size++;								//incremento la size e aggiungo gli elementi agli array
							words = (Word*) realloc(words, (size) * sizeof(Word));
							strcpy(words[size-1].parola, str);
							words[size-1].count = 1;
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
			printf("Parola: %s - counts: %d\n",words[i].parola,words[i].count);
		}
		free(words);
	}
}


/*
	Qui viene presa una stringa in input (la parola letta dal file) e viene restituita
	la stessa stringa, però solo con caratteri alfanumerici, in pratica con questa funzione
	è possibile analizzare in input anche file di testo relativi a libri o con segni di 
	punteggiatura tra le parole.
*/
int getWord(FILE *file, char* str){
	int i=0;
	char c;
	do{
		c = fgetc(file);
		printf("%c",c);
		if(isalnum(c))
			str[i++] = c;
	}while(isalnum(c));	
	str[i] = '\0';
	return i;
}



