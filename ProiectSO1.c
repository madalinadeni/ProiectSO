
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <sys/wait.h>
#include <time.h>

struct Date {
    char nume_d[300];
    char data_modificare[20];
    off_t marime;
};

int verificare_existenta(const char *path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

void actualizareSnapshot(const char *director, int nivel, const char *outputDir) {
    char snapshotPath[PATH_MAX];
    snprintf(snapshotPath, sizeof(snapshotPath), "%s/snapshot.txt", outputDir);
    
    FILE *snapshotFile = fopen(snapshotPath, "a");
    if (snapshotFile == NULL) {
        perror("Eroare la crearea fisierului de snapshot!");
        exit(1);
    }

    DIR *dir;
    struct dirent *intrare;
    if ((dir = opendir(director)) == NULL) {
        perror("Eroare la deschiderea directorului!");
        exit(1);
    }

    while ((intrare = readdir(dir)) != NULL) {
        if (strcmp(intrare->d_name, ".") != 0 && strcmp(intrare->d_name, "..") != 0) {
            char filePath[PATH_MAX];
            char linkPath[PATH_MAX+1], spatii[PATH_MAX];
            int n;

            memset(spatii, ' ', 2*nivel);
            spatii[2*nivel]='\0';

            snprintf(filePath, sizeof(filePath), "%s/%s", director, intrare->d_name);

            struct stat fileInfo;
            if (stat(filePath, &fileInfo) != 0) {
                perror("Eroare la preluarea informatiilor fisierului");
                continue;
            }

            struct tm *tm_info;
            char buffer[20];
            tm_info = localtime(&fileInfo.st_mtime);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

            if(S_ISDIR(fileInfo.st_mode)){
                actualizareSnapshot(filePath, nivel+1, outputDir);
            }

            if(S_ISLNK(fileInfo.st_mode)){
                n=readlink(filePath, linkPath, sizeof(linkPath));
                linkPath[n]='\0';
                fprintf(snapshotFile, "%s\t%s -> %s\t%s\t%ld\n", intrare->d_name, filePath, linkPath, buffer, fileInfo.st_size);
            }
            else{
                fprintf(snapshotFile, "%s\t%s\t%s\t%ld", intrare->d_name, filePath, buffer, fileInfo.st_size);
                if(fileInfo.st_mode & S_IXUSR || fileInfo.st_mode & S_IXGRP || fileInfo.st_mode & S_IXOTH)
                    fprintf(snapshotFile, "*");
                fprintf(snapshotFile, "\n");
            }
        }
    }

    closedir(dir);
    fclose(snapshotFile);
    printf("Snapshot-ul pentru directorul %s a fost generat cu succes!\n", director);
}

void childProcess(const char *director, int nivel, const char *outputDir) {
    pid_t pid = getpid();
    actualizareSnapshot(director, nivel, outputDir);
    printf("Procesul cu PID-ul %d s-a incheiat cu codul 0\n", pid);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 12) {
        fprintf(stderr, "Dati un numar de argumente cuprins intre 2 si 11");
        exit(1);
    }

    const char *outputDir = NULL;
    int dirIndex = -1;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputDir = argv[i + 1];
                dirIndex = i + 1;
                break;
            } else {
                fprintf(stderr, "Optiunea -o necesita un argument valid!");
                exit(1);
            }
        }
    }

    if (outputDir == NULL) {
        fprintf(stderr, "Lipseste directorul de iesire specificat prin optiunea -o!");
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        if (i == dirIndex) {
            ++i;
            continue;
        }

        const char *director = argv[i];
        if (!verificare_existenta(director)) {
            fprintf(stderr, "Directorul %s nu exista!\n", director);
            continue;
        }

        char outputPath[PATH_MAX];
        snprintf(outputPath, sizeof(outputPath), "%s/%s", outputDir, director);

        if (!verificare_existenta(outputPath)) {
            if (mkdir(outputPath, 0777) == -1) {
                perror("Eroare la crearea directorului de iesire!");
                exit(1);
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            childProcess(director, 1, outputPath);
        } else if (pid < 0) {
            perror("Eroare la crearea procesului copil!");
            exit(1);
        }
    }

    for (int i = 1; i < argc; ++i) {
        if (i == dirIndex) {
            ++i;
            continue;
        }

        wait(NULL);
    }

    return 0;
}

