#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

// Função para listar os arquivos
void list_files(int show_hidden, int long_format) {
    DIR *dir;
    struct dirent *ent;

    // Abre o diretório atual
    if ((dir = opendir(".")) != NULL) {
        // Lê cada entrada no diretório
        while ((ent = readdir(dir)) != NULL) {
            // Se não estivermos mostrando arquivos ocultos e a entrada começar com '.', pule
            if (!show_hidden && ent->d_name[0] == '.') {
                continue;
            }

            // Se estivermos em formato longo, imprima informações detalhadas
            if (long_format) {
                struct stat file_stat;
                if (stat(ent->d_name, &file_stat) == -1) {
                    perror("Erro ao obter informações do arquivo");
                    continue;
                }
                // Imprime as permissões e o tamanho do arquivo
                printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
                printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
                printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
                printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
                printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
                printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
                printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
                printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
                printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
                printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");
                printf("\t%ld\t%s\n", (long)file_stat.st_size, ent->d_name);
            } else {
                // Caso contrário, apenas imprima o nome do arquivo
                printf("%s\t", ent->d_name);
            }
        }
        closedir(dir); // Fecha o diretório após a leitura
    } else {
        // Se houver um erro ao abrir o diretório, exibe uma mensagem de erro
        perror("Erro ao abrir o diretório");
        exit(EXIT_FAILURE); // Sai do programa com falha
    }
}

// Função principal
int main(int argc, char *argv[]) {
    int show_hidden = 0; // Flag para mostrar arquivos ocultos
    int long_format = 0; // Flag para formato longo

    // Verifica se há argumentos de linha de comando
    if (argc > 1) {
        // Verifica cada argumento
        for (int i = 1; i < argc; i++) {
            // Se o argumento for "-a", definir show_hidden como verdadeiro
            if (strcmp(argv[i], "-a") == 0) {
                show_hidden = 1;
            }
            // Se o argumento for "-l", definir long_format como verdadeiro
            else if (strcmp(argv[i], "-l") == 0) {
                long_format = 1;
            }
            // Se o argumento for "-al" ou "-la", definir ambos os flags como verdadeiros
            else if (strcmp(argv[i], "-al") == 0 || strcmp(argv[i], "-la") == 0) {
                show_hidden = 1;
                long_format = 1;
            }
        }
    }

    // Chama a função para listar arquivos
    list_files(show_hidden, long_format);

    return 0; // Retorna 0 para indicar sucesso
}