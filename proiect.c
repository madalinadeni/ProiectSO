#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: %s <file1> <file2> ... <output_file>\n", argv[0]);
        return 1;
    }

    FILE *oF = fopen(argv[argc - 1], "wb");
    if (oF == NULL) {
        printf("Fisierul de iesire nu s-a putut deschide.\n");
        return 1;
    }

    for (int i = 1; i < argc - 1; i++) {
        FILE *inF = fopen(argv[i], "rb");
        if (inF == NULL) {
            printf("Fisierul de intrare nu s-a putut deschide.\n");
            fclose(oF);
            return 1;
        }

        int c;
        while ((c = fgetc(inF)) != EOF) {
            fputc(c, oF);
        }

        fclose(inF);
    }

    fclose(oF);

    printf("Fisierele s-au concatenat cu succes.\n");

    return 0;
}
