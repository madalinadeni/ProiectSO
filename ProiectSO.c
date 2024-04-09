#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

typedef struct{
    char nume_d[300];
    long ultima_modificare;
    long size;
}Date;

/*void afisareDate(struct Date *date){
    printf("Numele directorului este:%s\n", date->nume_d);
    printf("Ultima modificare:%ld", date->ultima_modificare);
    printf("Marimea:%ld", date->size);
    printf("\n");
}*/

int verificare_existenta(const char *path){
    struct stat buffer;
    return(stat(path, &buffer)==0);
}

void actualizareSnapshot(const char *director){
    FILE *snapshotFile = fopen("snapshot.txt", "w");
    if(snapshotFile == NULL){
        perror("Eroare la crearea fisierului de snapshot!");
        exit(1);
    }

    DIR *dir;
    struct dirent *intrare;
    if((dir=opendir(dir))==NULL){
        perror("Eroare la deschiderea directorului!");
        exit(1);
    }

    while((intrare=readdir(dir))!=NULL){
        if(strcmp(intrare->d_name, ".")!=0 && strcmp(intrare->d_name, "..") != 0){
            char filePath[PATH_MAX];
            snprintf(filePath, sizeof(filePath), "%s%s",director, intrare );

            struct stat fileInfo;
            if(stat(filePath, &fileInfo)!=0){
                perror("Eroare la preluarea informatiilor fisierului");
                continue;
            }

            fprintf(snapshotFile, "%s\t%ld\n", intrare->d_name, (long)fileInfo.st_mtime);
        }

        closedir(dir);
        fclose(snapshotFile);
    }
}

int comparare_fisiere(const struct Date *fileInfo, const char *director){
    char filePath[PATH_MAX];
    snprintf(filePath, sizeof(filePath), "%s%s", director, fileInfo->nume_d);

    struct stat currentInfo;
    if(stat(filePath, &currentInfo)!=0){
        return 1;
    }

    return (fileInfo->ultima_modificare != currentInfo.st_mtime);
}

void afisare_fisiere_modificate(const struct Date *fileInfo, int numarFisiere, const char *director){
    for(int i=0; i< numarFisiere; ++i){
        if(comparare_fisiere(&fileInfo[i], director)){
            printf("%s a fost modificat!\n", fileInfo[i].nume_d);
        }
    }
}

void incarcare_Snapshot(const char *director, struct Date **fileInfo, int *numarFisiere){
    FILE *snapshotFile = fopen("snapshot.txt", "r");
    if(snapshotFile == NULL){
        perror("Eroare la deschiderea fisierului snapshot!");
        exit(1);
    }

    int capacitate=11;
    *fileInfo=malloc(capacitate *sizeof(struct Date));
    if(*fileInfo == NULL){
        perror("Eroare la alocarea memoriei dinamice!");
        exit(1);
    }

    *numarFisiere=0;
    char nume_d[PATH_MAX];
    long int ultima_modificare;
    while(fscanf(snapshotFile, "%s\t%ld", nume_d, &ultima_modificare)!=EOF){
        if(*numarFisiere == capacitate){
            capacitate *=2;
            *fileInfo=realloc(*fileInfo, capacitate *sizeof(struct Date ));
            if(*fileInfo == NULL){
                perror("Realocarea memoriei a esuat!");
                exit(1);
            }
        }

        strcpy((*fileInfo)[*numarFisiere].nume_d, nume_d);
        (*fileInfo)[*numarFisiere].ultima_modificare=ultima_modificare;
        ++(*numarFisiere);

    }

    fclose(snapshotFile);
}

int main(int argc, char *argv[]){
    if(argc < 1 && argc >10){
        fprintf(stderr, "Dati un numar de argumente cuprins intre 1 si 10");
        exit(1);
    }

    const char *director = argv[1];
    if(!verificare_existenta(director)){
        fprintf(stderr, "Directorul nu exista!");
        exit(1);
    }

    if(!verificare_existenta("snapshot.txt")){
        printf("Snapshot-ul nu exista");
        actualizareSnapshot(director);
        printf("Snapshot-ul a fost creat.");
    }

    printf("Snapshot-ul exista!\n");
    struct Date *fileInfo;
    int numarFisiere;
    incarcare_Snapshot(director, &fileInfo, &numarFisiere);

    actualizareSnapshot(director);
    afisare_fisiere_modificate(fileInfo, numarFisiere, director);

    free(fileInfo);
    return 0;
}
/*
struct Date obtinereaMetadatelor(char *path){
    struct Date data;
    struct stat fileStat;

    if(stat(path, &fileStat)==-1){
        perror("Eroare la preluarea fisierelor!");
        exit(1);
    }

    strcpy(data.nume, path);
    data.size=fileStat.st_size;
    data.ultima_modificare=fileStat.st_mtime;
    return data;
}

void parcurgereDirector(char *nume_director, int nivel){
    DIR *director;
    struct dirent *intrare;
    char *nume;
    struct stat info;
    char cale[PATH_MAX], cale_link[PATH_MAX+1], spatii[PATH_MAX];
    int n;

    memset(spatii, ' ', 2*nivel);
    spatii[2*nivel]='\0';

    if(!(director = opendir(nume_director))){
        printf("%s: ", nume_director);
        fflush(stdout);
        perror("Eroare la deschiderea directorului!");
        exit(1);
    }

    printf("%s DIR %s:\n", spatii, nume_director);

    while((intrare==readdir(director))>0){
        nume=intrare->d_name;
        if(strcmp(nume, ".")==0 || strcmp(nume, "..")==0)
        continue;

        snprintf(cale, sizeof(cale), "%s/%s", nume_director, nume);

        if(lstat(cale, &info)<0){
            printf("%s: ", cale);
            fflush(stdout);
            perror("Eroare la lstat!");
            exit(1);
        }

        if(S_ISDIR(info.st_mode)){
            parcurgereDirector(cale, nivel+1);
        }
        if(S_ISLNK(info.st_mode)){
            n=readlink(cale, cale_link, sizeof(cale_link));
            cale_link[n]='\0';
            printf("%s %s -> %s\n", spatii, cale, cale_link);
        }
        else{
            printf("%s %s", spatii, cale);
            if(info.st_mode & S_IXUSR || info.st_mode & S_IXGRP || info.st_mode & S_IXOTH)
                printf("*");
            printf("\n");
        }
    }
    closedir(director);
}*/
/*
int main(int argc, char *argv[]){
    if(argc  < 0 && argc > 11){
        printf("Numarul de argumente trebuie sa fie cuprins intre 1 si 10");
        exit(1);
    }

    parcurgereDirector(argv[1], 0);

    return 0;
}*/
