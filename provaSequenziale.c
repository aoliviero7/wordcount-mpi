#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<stdlib.h>
#include<time.h>
int main(int argc, char**argv) {

char** parola;
int* countParola;

clock_t start,end;
double tempo;
start = clock();

DIR* dir;
struct dirent* Dirent;
FILE* file;
dir = opendir("txt");
int sommabyte = 0;
int* fileDimension;
int count = 0;
int size=0;
countParola = (int*)malloc(sizeof(int));
parola = (char**)malloc(sizeof(char*));
parola[0] = (char*)malloc(100*sizeof(char));
char stringa[500];
if(dir!=NULL) {

	while((Dirent=readdir(dir))!=NULL) {
		count++;
		}
	fileDimension = (int*)malloc(sizeof(int)*count);
		
	seekdir(dir,0);
	count = 0;
	while((Dirent=readdir(dir))!=NULL) {
		//printf("file : %s tipo %d \n",Dirent->d_name,Dirent->d_type);
		if(Dirent->d_type==8) {
			char stringa[100] ="txt/";
			strcat(stringa,Dirent->d_name);
			//printf("%s\n",stringa);
			file=fopen(stringa,"r");
			if(file!=NULL) {
				while(!feof(file)) {
					fscanf(file,"%s",stringa);
					int verifica = 0;
					int indice = 0;
					for(int i=0;i<size;i++) {
						if(strcmp(stringa,parola[i])==0) {
							verifica = 1;
							indice = i;
						}
					}
					if(verifica==1) {
						countParola[indice]++;
					}
					else {
						size++;
						countParola = (int*)realloc(countParola,sizeof(int)*size);
						countParola[size-1] = 1;
						parola = (char**)realloc(parola,size*sizeof(char*));
						parola[size-1] = (char*)malloc(100*sizeof(char));
						strcpy(parola[size-1],stringa);
						
						
					}
				}
				fclose(file);
			}
		}
		
	}
	
	
	for(int i=0;i<size;i++) {
		printf("word : %s count: %d \n",parola[i],countParola[i]);
	}
	free(parola);
	free(countParola);	
}

closedir(dir);
end = clock();
tempo=((double)(end-start))/CLOCKS_PER_SEC;
printf("tempo %f secondi\n",tempo);

}








