//#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/time.h>

#define FOLDER "txt"

typedef struct{
	char parola[100];
	int count;
}Word;

int indexOf(Word *words, char* str, int size);
int getWord(FILE *file, char* str);
int compare(const void * a, const void * b);

int main(int argc, char** argv) {
	struct timeval start;
   	struct timeval end;
   	float elapsed;
	gettimeofday(&start, 0);
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	Word* words;
	words = (Word*) malloc(sizeof(Word));
	int size = 0, totalWords = 0;
	
	directory = opendir(FOLDER);
	if(directory){
		while(Dirent=readdir(directory)){
			char filepath[100] = FOLDER;
			strcat(filepath, "/");
			strcat(filepath, Dirent->d_name);
			//printf("Filepath: %s\n",filepath);
			if(Dirent->d_type==8){
				file = fopen(filepath, "r");
				printf("Filename: %s\n",Dirent->d_name);
				if(file){									
					while(!feof(file)){							//scansiono il file
						char str[100]={};
						if(!getWord(file, str))					//prendo una parola	
							continue;
						totalWords++;
						int index = 0;
						int i = indexOf(words, str, size);		//cerco nell'array se è già presente la parola
						if(i!=-1)								
							words[i].count++;					//se sì, incremento il suo contatore
						else{									
							size++;								//se no, incremento la size e aggiungo gli elementi agli array
							words = (Word*) realloc(words, (size) * sizeof(Word));
							strcpy(words[size-1].parola, str);
							words[size-1].count = 1;
						}
					}
				}
				fclose(file);
			}
		}/*
		for(int i=0; i<size; i++){
			printf("Parola: %s - counts: %d\n",words[i].parola,words[i].count);
		}*/
		printf("Total words: %d - different words: %d\n",totalWords,size);
	}
	else
		printf("Directory non leggibile\n");
	closedir(directory);
	
	qsort(words,size,sizeof(Word),compare);						//ordino i dati
	FILE *fpt;
	fpt = fopen("ResultsSequential.csv", "w+");					//e li scrivo su un file csv
	fprintf(fpt,"Word, Count\n");
	for(int i=0; i<size; i++)
		fprintf(fpt,"%s, %d\n",words[i].parola,words[i].count);

	gettimeofday(&end, 0);
	elapsed = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
	printf("Code executed in %.2f milliseconds.\n", elapsed);

	free(words);
	fclose(fpt);
}

int indexOf(Word *words, char* str, int size){
	for(int i=0; i<size; i++)					//cerco nell'array se è già presente la parola
		if(!strcmp(str, words[i].parola))		//se le parole sono uguali
			return i;							//restituisco l'indice
	return -1;
}

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

int compare(const void * a, const void * b){
	return ((Word*)b)->count - ((Word*)a)->count;
}
