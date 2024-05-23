#include <stdio.h>
#include <stdlib.h> // Para a função exit()
// Definindo cores ANSI
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"

int main(int argc, char *argv[]) {
    // Verifica se foi fornecido um arquivo como argumento
    if (argc != 2 && argc != 4) {
        printf( ANSI_COLOR_GREEN "Uso: %s <arquivo_origem> ou %s <arquivo_origem> > <arquivo_destino>\n" ANSI_COLOR_RESET, argv[0], argv[0]);
        return 1;
    }

    // Abre o arquivo para leitura binária
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        perror("Erro ao abrir o arquivo de origem");
        return 1;
    }

    // Se houver redirecionamento para um novo arquivo
    if (argc == 4 && argv[2][0] == '>' && argv[2][1] == '\0') {
        // Abre o arquivo de destino para escrita binária
        FILE *destFile = fopen(argv[3], "wb");
        if (destFile == NULL) {
            perror("Erro ao abrir o arquivo de destino");
            fclose(file);
            return 1;
        }

        // Lê o conteúdo do arquivo de origem e escreve no arquivo de destino
        int ch;
        while ((ch = fgetc(file)) != EOF) {
            fputc(ch, destFile);
        }

        // Fecha o arquivo de destino
        fclose(destFile);
    } else { // Se não houver redirecionamento, imprime na saída padrão
        // Lê e imprime o conteúdo do arquivo de origem
        int ch;
        while ((ch = fgetc(file)) != EOF) {
            putchar(ch);
        }
    }

    // Fecha o arquivo de origem
    fclose(file);

    return 0;
}
