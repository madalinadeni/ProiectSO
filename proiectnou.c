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
    return (lstat(path, &buffer) == 0);
}

int verificare_drepturi(const char *path) {
    struct stat fileInfo;
    if (lstat(path, &fileInfo) != 0) {
        perror("Eroare la preluarea informatiilor fisierului");
        return 0;
    }

    if (!(fileInfo.st_mode & S_IRUSR) || !(fileInfo.st_mode & S_IWUSR) || !(fileInfo.st_mode & S_IXUSR)) {
        return 1;
    }

    return 0;
}

int numar_fisiere_corupte_mutate = 0; 

int izolare_fisier(const char *path, const char *izolatedSpaceDir) {
    char destinationPath[PATH_MAX];
    snprintf(destinationPath, sizeof(destinationPath), "%s/%s", izolatedSpaceDir, strrchr(path, '/') + 1);

    if (!verificare_drepturi(path)) {
        return 0;
    }

    pid_t pid = fork();
    if (pid == 0) {
        execl("/bin/bash", "bash", "verify_foor_malicious.sh", path, NULL);
        perror("Eroare la rularea scriptului de verificare pentru fisierul corupt/malitios");
        exit(1);
    } else if (pid < 0) {
        perror("Eroare la crearea procesului copil");
        return 1;
    } else {
        int status;
        wait(&status); 

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status != 0) {
      
                printf("Mutare fisier corupt: %s -> %s\n", path, destinationPath);
                if (rename(path, destinationPath) != 0) {
                    perror("Eroare la mutarea fisierului in directorul izolat");
                    return 1;
                } else {
                    printf("Fisierul %s a fost mutat cu succes in directorul izolat %s!\n", path, izolatedSpaceDir);
                    numar_fisiere_corupte_mutate++;
                    return 1;
                }
            } else {
                if (rename(path, destinationPath) != 0) {
                    perror("Eroare la mutarea fisierului in directorul izolat");
                    return 1;
                } else {
                    printf("Fisierul %s a fost mutat cu succes in directorul izolat %s!\n", path, izolatedSpaceDir);
                    numar_fisiere_corupte_mutate++;
                    return 1;
                }
            }
        } else {
            fprintf(stderr, "Procesul copil a fost terminat cu o eroare!\n");
            return 1;
        }
    }
}

int actualizareSnapshot(const char *director, int nivel, const char *outputDir, const char *izolatedSpaceDir) {
    char snapshotPath[PATH_MAX];
    snprintf(snapshotPath, sizeof(snapshotPath), "%s/snapshot.txt", outputDir);
    
    FILE *snapshotFile = fopen(snapshotPath, "a");
    if (snapshotFile == NULL) {
        perror("Eroare la crearea fisierului de snapshot!");
        exit(1);
    }

    int numar_fisiere_corupte = 0; 

    DIR *dir;
    struct dirent *intrare;
    if ((dir = opendir(director)) == NULL) {
        perror("Eroare la deschiderea directorului!");
        exit(1);
    }

    while ((intrare = readdir(dir)) != NULL) {
        if (strcmp(intrare->d_name, ".") != 0 && strcmp(intrare->d_name, "..") != 0) {
            char filePath[PATH_MAX];
            char linkPath[PATH_MAX+1];
            int n;

            snprintf(filePath, sizeof(filePath), "%s/%s", director, intrare->d_name);

            struct stat fileInfo;
            if (lstat(filePath, &fileInfo) != 0) {
                perror("Eroare la preluarea informatiilor fisierului");
                continue;
            }

            struct tm *tm_info;
            char buffer[20];
            tm_info = localtime(&fileInfo.st_mtime);
            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", tm_info);

            if(S_ISDIR(fileInfo.st_mode)){
                numar_fisiere_corupte += actualizareSnapshot(filePath, nivel+1, outputDir, izolatedSpaceDir);
            }

            if(S_ISLNK(fileInfo.st_mode)){
                n = readlink(filePath, linkPath, sizeof(linkPath));
                linkPath[n] = '\0';
                fprintf(snapshotFile, "%s\t%s -> %s\t%s\t%ld\n", intrare->d_name, filePath, linkPath, buffer, fileInfo.st_size);
            }
            else{
                if(fileInfo.st_mode & S_IXUSR || fileInfo.st_mode & S_IXGRP || fileInfo.st_mode & S_IXOTH) {
                    fprintf(snapshotFile, "%s\t%s\t%s\t%ld*\n", intrare->d_name, filePath, buffer, fileInfo.st_size);
                    if (izolare_fisier(filePath, izolatedSpaceDir)) {
                        numar_fisiere_corupte++; 
                    }
                } else {
                    fprintf(snapshotFile, "%s\t%s\t%s\t%ld\n", intrare->d_name, filePath, buffer, fileInfo.st_size);
                }
            }

            if(verificare_drepturi(filePath)){
                izolare_fisier(filePath, izolatedSpaceDir);
            }
        }
    }

    closedir(dir);
    fclose(snapshotFile);
    printf("Snapshot-ul pentru directorul %s a fost generat cu succes!\n", director);

    return numar_fisiere_corupte; 
}

void childProcess(const char *director, int nivel, const char *outputDir, const char *izolatedSpaceDir) {
    pid_t pid = getpid();
    actualizareSnapshot(director, nivel, outputDir, izolatedSpaceDir);
    printf("Procesul cu PID-ul %d s-a incheiat cu codul %d.\n", pid, numar_fisiere_corupte_mutate);
    exit(numar_fisiere_corupte_mutate); 
}

int main(int argc, char *argv[]) {
    if (argc <5  || argc > 15) {
        fprintf(stderr, "Numarul invalid de argumente! Utilizare: ./program_exe -o output_dir -s izolated_space_dir dir1 dir2 dir3\n");
        exit(1);
    }
    
    const char *outputDir = NULL;
    const char *izolatedSpaceDir = NULL;
    int dirIndex = -1;
    int izolatedIndex = -1;

    for (int i = 1; i < argc; i++){
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

    for(int j=1; j < argc; j++){
        if (strcmp(argv[j], "-s") == 0) {
            if (j + 1 < argc) {
                izolatedSpaceDir = argv[j + 1];
                izolatedIndex = j + 1;
                break;
            } else {
                fprintf(stderr, "Optiunea -s necesita un argument valid!");
                exit(1);
            }
        }
    }
    
    if (outputDir == NULL) {
        fprintf(stderr, "Lipseste directorul de iesire specificat prin optiunea -o!");
        exit(1);
    }

    if (izolatedSpaceDir == NULL) {
        fprintf(stderr, "Lipseste directorul izolat specificat prin optiunea -s!");
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (i == dirIndex || i == izolatedIndex) {
            i++;
            continue;
        }

        const char *director = argv[i];
        if (strcmp(director, "-o") == 0) {
            continue;
        }

        if (!verificare_existenta(director)) {
            fprintf(stderr, "Directorul %s nu exista!\n", director);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0) {
            childProcess(director, 1, outputDir, izolatedSpaceDir);
            exit(0); 
        } else if (pid < 0) {
            perror("Eroare la crearea procesului copil!");
            exit(1);
        }
    }

    int total_fisiere_corupte = 0; 

    for (int i = 1; i < argc; i++) {
        if (i == dirIndex || i == izolatedIndex) {
            i++;
            continue;
        }

        int status;
        wait(&status); 

        if (WIFEXITED(status)) {
            total_fisiere_corupte += WEXITSTATUS(status); 
        } else {
            fprintf(stderr, "Procesul copil a fost terminat cu o eroare!\n");
        }
    }


    return 0;
}
