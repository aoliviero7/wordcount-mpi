//#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <dirent.h>

int counttotalwords(FILE *file);
char* myscanf(char* str);

int main(int argc, char** argv) {
	DIR *directory;
	FILE *file;
	struct dirent *Dirent;
	int *filewords;
	int countfiles = 0, totalwords = 0;
	
	directory = opendir("txt");
	if(directory){
		while(Dirent=readdir(directory))
			countfiles++;
		seekdir(directory, 0);
		filewords = (int*) malloc(sizeof(int) * countfiles);
		while(Dirent=readdir(directory)){
			int i = 0;
			char filepath[100] = "txt/";
			strcat(filepath, Dirent->d_name);
			printf("Filepath: %s\n",filepath);
			if(Dirent->d_type==8 && !strcmp(Dirent->d_name,"prova.txt")){
				file = fopen(filepath, "r");
				printf("Filename: %s\n",Dirent->d_name);
				if(file)
					filewords[i] = counttotalwords(file);
				totalwords+=filewords[i++];
			}
		}
		printf("Total words: %d\n",totalwords);
	}
	else
		printf("Directory non leggibile\n");
	closedir(directory);
}

int counttotalwords(FILE *file){
	char str[100];
	int count = 0;
	while(!feof(file)){
		fscanf(file,"%s ",str);
		if(myscanf(str))
			count++;
		//printf("%s\n",str);
	}
	fclose(file);
	printf("Words: %d\n",count);
	return count;
}

char* myscanf(char* str){
	int len = strlen(str);
	printf("Lenght: %d ",len);
	printf("pre-word: %s ",str);
	char *word;
	word = (char*) malloc((sizeof(char) * len) + 1);
	for(int i=0, j=0;i<=len;i++)
		if((str[i]>='a' && str[i]<='z') || (str[i]>='A' && str[i]<='Z') || (str[i]>='0' && str[i]<='9') || (str[i]>='à' && str[i]<='ù'))
			word[j++] = str[i];
	printf("post-word: %s\n",word);
	if(strcmp(word,""))
		return word;
	else
		return NULL;
}
