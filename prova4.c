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

Word* wordCount(int inizio, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank,int *size);
Word* chunkAndCount(int myrank, int totalByte, fileStats* myFiles, int countFiles, int p, int *size);
int getWord(FILE *file, char* str);
int indexOf(Word *words, char* str, int size);
fileStats* fileScan(int* countFiles, int* totalByte);

int main(int argc, char** argv) {
	MPI_Init(NULL, NULL);			//inizializzo MPI
    int myrank,p=0;
    MPI_Status status;
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

	int countFiles = 0, totalByte = 0;
	fileStats *myFiles = fileScan(&countFiles, &totalByte);								//calcolo numero di file e dimensioni
	printf("count file %d, totalB %d\n", countFiles, totalByte);
	if(myrank==0)
		for(int i=0; i<countFiles; i++)
			printf("Nome file %d: %s, Byte %d\n", i, myFiles[i].name, myFiles[i].size);
	
	int size = 0;																		//dimensione dell'array di Word che calcolerò
	Word* words = chunkAndCount(myrank, totalByte, myFiles, countFiles, p, &size);		//calcolo l'array di word
	int sizeRecv[p], sizeTotal = 0, max = 0;
	MPI_Gather(&size, 1, MPI_INT, sizeRecv, 1, MPI_INT, 0, MPI_COMM_WORLD);				//comunico al master le dimensioni di ogni array di Word locale
	if(myrank==0){
		for(int i=0; i<p; i++){
			printf("[MASTER] Dimensione: %d - rank: %d\n",sizeRecv[i],i);
			sizeTotal += sizeRecv[i];
			if(sizeRecv[i]>max)
				max = sizeRecv[i];
		}
	}
	
	MPI_Datatype wordtype, oldtypes[2], parolatype;   									//variabili richieste per la struttura
    int blockcounts[2];
    MPI_Aint offsets[2], lb, extent;
	MPI_Type_contiguous(100, MPI_CHAR, &parolatype);
    MPI_Type_commit(&parolatype);
    offsets[0] = 0;																		//setup parola
    oldtypes[0] = parolatype;
    blockcounts[0] = 1;
    MPI_Type_get_extent(parolatype, &lb, &extent);										//setup count
    offsets[1] = extent;
    oldtypes[1] = MPI_INT;
    blockcounts[1] = 2;
    MPI_Type_create_struct(2, blockcounts, offsets, oldtypes, &wordtype);				//definizione struttura Word
    MPI_Type_commit(&wordtype);
	
	int sizeWord;								
    MPI_Type_size(wordtype, &sizeWord);													//dimensione di una singola Word

	char sendMessage[size * sizeWord];													//pacchetto che ogni processo invia al master
	char recvMessage[sizeTotal * sizeWord];												//pacchetto che riceve il master (pacchetti singoli concatenati)
	Word wordsRecv[p][max];																//array degli istogrammi locali dei processi
	int displacements[p], sizeMessage[p], sizeMessageRecv = 0, position = 0;			//variabili richieste per l'invio
	if(myrank==0){
		for(int i=0; i<p; i++){															//il master calcola indici e dimensioni
			sizeMessage[i] = sizeRecv[i] * sizeWord;
			sizeMessageRecv += sizeMessage[i];
			displacements[i] = (i==0) ? 0 : displacements[i-1] + sizeMessage[i-1];
		}
	}
    for (int i=0; i<size; i++) 															//ogni processo fa MPI_Pack dei suoi dati calcolati
        MPI_Pack(&words[i], 1, wordtype, sendMessage, sizeWord*size, &position, MPI_COMM_WORLD);
	
	MPI_Gatherv(sendMessage, sizeWord*size, MPI_PACKED, recvMessage, sizeMessage, displacements, MPI_PACKED, 0, MPI_COMM_WORLD);	//si inviano tutti i dati al master
	if(myrank==0){
		position = 0;
		for (int i=0; i<p; i++)
			for (int j=0; j<sizeRecv[i]; j++) 
				MPI_Unpack(recvMessage, sizeMessageRecv, &position, &wordsRecv[i][j], 1, wordtype, MPI_COMM_WORLD);					//il master spacchetta tutti i dati
		for(int i=0; i<p; i++){
			printf("Processo: %d\n",i);
			for(int j=0; j<sizeRecv[i]; j++){
				printf("Parola: %s - counts: %d\n",wordsRecv[i][j].parola,wordsRecv[i][j].count);
			}
		}
	}

	MPI_Type_free(&parolatype);
    MPI_Type_free(&wordtype);
	free(words);
	free(myFiles);
	MPI_Finalize();
}


/*
	Qui calcolo la porzione di byte che ogni processo deve elaborare
	Es: il processo 3 lavora dal byte 400 del file 5 fino al byte 150 del file 7
	Dopodiché viene chiamato il metodo wordCount
*/
Word* chunkAndCount(int myrank, int totalByte, fileStats* myFiles, int countFiles, int p, int *size){
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
	
	return wordCount(inizio, inizioFile, fineFile, resto, byteProcess, myFiles, myrank, size);
}


/*
	Qui ogni processo si calcola la frequenza delle parole delle porzioni dei file che gli spettano
*/
Word* wordCount(int inizio, int inizioFile, int fineFile, int resto, int byteProcess, fileStats* myFiles, int myrank, int *size){
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	int countByte = 0;
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
						char str[100]={};
						int prima = ftell(file);
						if(!getWord(file, str))					//prendo una parola	
							continue;																
						int dopo = ftell(file);
						//printf("Stringa letta: %ls \n",str);
						int salto = 0;												//flag che mi indica se la parola la devo saltare	
						if(iFile == inizioFile && prima == inizio && inizio!=0){	//se sto leggendo la mia prima parola (prima==inizio) del primo file che mi è stato assegnato (iFile==inizioFile),
																					//e non mi trovo all'inizio del file (inizio!=0) (NB: rank 0 non lo farà mai dato che il suo inizio=0)
							fseek(file, inizio-1, SEEK_SET);						//mi sposto di un byte(carattere) indietro
							char c;
							c = fgetc(file);										//leggo il carattere precedente a quello che mi spetta realmente	
							if(!isspace(c))											//se il carattere precedente non è uno spazio (quindi fa parte di una parola)
								salto = 1;											//devo saltare la parola poiché è già stata letta dal processo precedente, altrimenti devo leggerla	
							fseek(file, dopo, SEEK_SET);							//mi riposiziono in modo giusto	
						}															
						countByte+=(dopo-prima);				//aggiorno i byte analizzati
						if(salto)								//se non devo leggere la parola
							continue;							//vado alla prossima iterazione perché la parola è già stata analizzata
						
						int flag = 0;
						int i = indexOf(words, str, *size);		//cerco nell'array se è già presente la parola
						if(i!=-1)								
							words[i].count++;					//se sì, incremento il suo contatore
						else{									
							*size=*size+1;						//se no, incremento la size e aggiungo gli elementi agli array
							words = (Word*) realloc(words, (*size) * sizeof(Word));
							strcpy(words[*size-1].parola, str);
							words[*size-1].count = 1;
						}
					}else
						break;
				}
			}
			fclose(file);
		}
		MPI_Barrier(MPI_COMM_WORLD);
		sleep(myrank);
		/*printf("Processo: %d \n",myrank);
		for(int i=0; i<*size; i++){
			printf("Parola: %s - counts: %d\n",words[i].parola,words[i].count);
		}*/
		//printf("Processo: %d - dimensione struct: %ld\n",myrank,(*size) * sizeof(Word));

		return words;
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
		//printf("%c ",c);
		if(isalnum(c))
			str[i++] = tolower(c);
	}while(isalnum(c));	
	str[i] = '\0';
	return i;
}

int indexOf(Word *words, char* str, int size){
	for(int i=0; i<size; i++)					//cerco nell'array se è già presente la parola
		if(!strcmp(str, words[i].parola))		//se le parole sono uguali
			return i;							//restituisco l'indice
	return -1;
}


/*
	Qui vengono calcolati i byte totali dei file da esaminare, il numero totale dei file
	e nell'array di fileStats restituito si inserisce per ogli file il suo nome e la sua dimensione.
*/
fileStats* fileScan(int* countFiles, int* totalByte){
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	int *fileByte;
	fileStats* myFiles;
	directory = opendir(FOLDER);
	if(directory){
		while(Dirent=readdir(directory))									//conto i file
			*countFiles = *countFiles + 1;
		seekdir(directory, 0);
		myFiles = (fileStats*) malloc(sizeof(fileStats) * (*countFiles));	//alloco la struttura con il numero dei file
		*countFiles = 0;
		while(Dirent=readdir(directory)){
			char filepath[100] = FOLDER;
			strcat(filepath, "/");
			strcat(filepath, Dirent->d_name);
			if(Dirent->d_type==8){											//regoular file
				file = fopen(filepath, "r");								//accedo in lettura
				strcpy(myFiles[*countFiles].name,Dirent->d_name);			//salvo il nome
				if(file){									
					fseek(file, 0L, SEEK_END);								//conto i byte dei file posizionandomi alla fine
					myFiles[*countFiles].size = ftell(file);				//salvo la dimensione
					*totalByte = *totalByte + myFiles[*countFiles].size;	//aggiorno il contatore totale dei byte
					*countFiles = *countFiles + 1;
				}
				fclose(file);
			}
		}
	}
	else
		printf("Directory non leggibile\n");
	closedir(directory);
	return myFiles;
}

