//#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dirent.h>

#define FOLDER "txt"

int main(int argc, char** argv) {
	struct timeval start;
   	struct timeval end;
   	float elapsed;
	gettimeofday(&start, 0);
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	char **parole;
	int *count;
	int size = 0;
	count = (int*) malloc(sizeof(int));
	parole = (char**) malloc(sizeof(char*));
	
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
					char str[100];									
					while(!feof(file)){							//scansiono il file
						fscanf(file,"%s \n", str);				//prendo una parola
						
						int index = 0;
						int flag = 0;
						for(int i=0; i<size; i++){				//cerco nell'array se è già presente la parola
							if(!strcmp(str, parole[i])){		//se le parole sono uguali
								flag = 1;						//setto il flag a true
								index = i;						//mi salvo l'indice della parola
							}
						}
						if(flag)								//flag = 1
							count[index]++;						//la parola esiste già e incremento il suo contatore
						else{									//flag = 0
							size++;								//incremento la size e aggiungo gli elementi agli array
							parole = (char**) realloc(parole, size * sizeof(char*));
							parole[size-1] = (char*) malloc(100 * sizeof(char));
							count = (int*) realloc(count, size * sizeof(int));
							strcpy(parole[size-1], str);
							count[size-1] = 1;
						}
					}
				}
				fclose(file);
			}
		}
		/*for(int i=0; i<size; i++){
			printf("Parola: %s - counts: %d\n",parole[i],count[i]);
		}*/
		free(parole);
		free(count);
	}
	else
		printf("Directory non leggibile\n");
	closedir(directory);
	gettimeofday(&end, 0);
	elapsed = (end.tv_sec - start.tv_sec) * 1000.0f + (end.tv_usec - start.tv_usec) / 1000.0f;
	printf("Code executed in %.2f milliseconds.\n", elapsed);
}

