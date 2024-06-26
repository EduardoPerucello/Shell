#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>

#define MAXCOM 1000 
#define MAXLIST 100
#define clear() printf("\033[H\033[J")
#define BUFSIZE 1024
#define MAX_PATHS 100
#define MAX_PATH_LENGTH 1024
#define HOST_NAME_MAX 255

// Adicionando cores ANSI
#define ANSI_COLOR_RED      "\x1b[31m"
#define ANSI_COLOR_BLUE     "\x1b[34m"
#define ANSI_COLOR_GREEN    "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_RESET    "\x1b[0m"

// Definindo os códigos de cores
#define ANSI_COLOR_RED_BRIGHT "\x1b[91m"
#define ANSI_COLOR_YELLOW_BRIGHT "\x1b[93m"

char* paths[MAX_PATHS];
int path_count = 0;
char current_dir[BUFSIZE];
char hostname[HOST_NAME_MAX];
char *username;

void init_current_dir() {
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) { 
        fprintf(stderr, ANSI_COLOR_RED "Erro ao obter o diretório atual\n" ANSI_COLOR_RESET); 
        exit(EXIT_FAILURE);
    }
}

char *read_line(void) {
    int bufsize = BUFSIZE; // Tamanho inicial do buffer
    int position = 0; // Posição atual dentro do buffer
    char *buffer = malloc(sizeof(char) * bufsize); // Aloca memória para o buffer inicial
    int c; // Variável para armazenar cada caractere lido

    if (!buffer) { // Verifica se a alocação de memória foi bem-sucedida
        fprintf(stderr, ANSI_COLOR_RED "Erro de alocação de memória\n" ANSI_COLOR_RESET); // Em caso de falha, exibe uma mensagem de erro e encerra o programa
        exit(EXIT_FAILURE);
    }

    while (1) { // Loop principal para ler os caracteres da entrada
        c = getchar(); // Lê um caractere da entrada padrão (teclado)
        if (c == EOF || c == '\n') { // Verifica se o caractere lido é o fim do arquivo (EOF) ou uma nova linha
            buffer[position] = '\0'; // Adiciona um caractere nulo ao final da string para indicar o término
            return buffer; // Retorna o buffer contendo a linha lida
        } else {
            buffer[position] = c; // Armazena o caractere lido no buffer na posição atual
        }
        position++; // Incrementa a posição dentro do buffer

        if (position >= bufsize) { // Verifica se o tamanho atual do buffer foi excedido
            bufsize += BUFSIZE; // Aumenta o tamanho do buffer
            buffer = realloc(buffer, bufsize); // Realoca memória para o buffer com o novo tamanho
            if (!buffer) { // Verifica se a realocação de memória foi bem-sucedida
                fprintf(stderr, ANSI_COLOR_RED "Erro de alocação de memória\n" ANSI_COLOR_RESET); // Em caso de falha, exibe uma mensagem de erro e encerra o programa
                exit(EXIT_FAILURE);
            }
        }
    }
}

void retirarEspaco(char* str, char** comandos) {
    int i = 0; // Variável para controlar a posição dos comandos
    char *token = strtok(str, " "); // Obtém o primeiro token (palavra) da string, delimitado por espaços
    // Enquanto houver tokens
    while (token != NULL) {
        comandos[i++] = token; // Armazena o token atual no array de comandos e incrementa a posição
        token = strtok(NULL, " "); // Obtém o próximo token da string
    }
    comandos[i] = NULL; // Adiciona um último elemento nulo ao final do array de comandos
}


void add_path(const char* path) {
    if (access(path, F_OK) == -1) { // Verifica se o caminho não existe
        fprintf(stderr, ANSI_COLOR_RED "Caminho não existe: %s\n" ANSI_COLOR_RESET, path); // Exibe uma mensagem de erro se o caminho não existe
        return; // Retorna sem adicionar o caminho
    }

    if (path_count < MAX_PATHS) { // Verifica se ainda há espaço para adicionar mais caminhos
        paths[path_count] = strdup(path); // Adiciona o caminho ao array de caminhos
        path_count++; // Incrementa o contador de caminhos
    } else {
        fprintf(stderr, ANSI_COLOR_RED "Limite de caminhos atingido\n" ANSI_COLOR_RESET); // Exibe uma mensagem de erro se o limite de caminhos foi atingido
    }
}


void remove_path(const char* path) {
    for (int i = 0; i < path_count; i++) { // Percorre todos os caminhos armazenados
        if (strcmp(paths[i], path) == 0) { // Verifica se o caminho atual é igual ao caminho que deseja remover
            free(paths[i]); // Libera a memória alocada para o caminho
            for (int j = i; j < path_count - 1; j++) { // Move os caminhos subsequentes uma posição para trás para preencher a lacuna
                paths[j] = paths[j + 1];
            }
            path_count--; // Decrementa o contador de caminhos
            return; // Retorna após remover o caminho
        }
    }
    fprintf(stderr, ANSI_COLOR_GREEN "Caminho não encontrado: %s\n" ANSI_COLOR_RESET, path); // Se o caminho não foi encontrado, exibe uma mensagem de erro
}


void list_paths() {
    for (int i = 0; i < path_count; i++) {
        printf("%s\n", paths[i]);
    }
}

void executaEcho(char** parsed) {
    for (int i = 1; parsed[i] != NULL; i++) { // Começa do índice 1 para pular o comando 'echo'
        printf("%s ", parsed[i]); // Imprime cada argumento seguido de um espaço
    }
    printf("\n"); // Adiciona uma nova linha ao final da saída
}


int builtinComandos(char** parsed) {
    if (strcmp(parsed[0], "cd") == 0) { // Verifica se o comando é "cd"
        if (parsed[1] == NULL || strcmp(parsed[1], "~") == 0) { // Verifica se nenhum diretório é especificado ou se é especificado "~"
            struct passwd *pw = getpwuid(getuid()); // Obtém o diretório home do usuário
            if (pw == NULL) { // Verifica se houve erro na obtenção do diretório home
                perror("getpwuid");
                return 1;
            }
            if (chdir(pw->pw_dir) != 0) { // Muda para o diretório home do usuário
                perror("chdir");
                return 1;
            }
        } else if (strcmp(parsed[1], "..") == 0) { // Verifica se o diretório é ".."
            if (chdir("..") != 0) { // Muda para o diretório pai
                perror("chdir");
                return 1;
            }
        } else {
            if (chdir(parsed[1]) != 0) { // Muda para o diretório especificado
                perror("chdir");
                return 1;
            }
        }
        init_current_dir(); // Inicializa o diretório atual
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) { // Verifica se o comando é "exit"
        printf("\nGoodbye\n"); // Exibe mensagem de despedida
        exit(EXIT_SUCCESS); // Encerra o programa com sucesso
    } else if(strcmp(parsed[0], "clear") == 0){ // Verifica se o comando é "clear"
        printf("\033[H\033[J"); // Limpa a tela do terminal
    } else if (strcmp(parsed[0], "path") == 0) { // Verifica se o comando é "path"
        if (parsed[1] == NULL) { // Verifica se nenhum argumento é especificado
            list_paths(); // Lista todos os caminhos armazenados
        } else if (strcmp(parsed[1], "add") == 0 && parsed[2] != NULL) { // Verifica se o argumento é "add" e se há caminhos especificados
            for (int i = 2; parsed[i] != NULL; i++) { // Adiciona os caminhos especificados
                add_path(parsed[i]);
            }
        } else if (strcmp(parsed[1], "remove") == 0 && parsed[2] != NULL) { // Verifica se o argumento é "remove" e se há caminhos especificados
            for (int i = 2; parsed[i] != NULL; i++) { // Remove os caminhos especificados
                remove_path(parsed[i]);
            }
        } else {
            fprintf(stderr, ANSI_COLOR_GREEN "Uso: path [add|remove] [caminho1 caminho2 ...]\n" ANSI_COLOR_RESET); // Exibe mensagem de uso correto do comando "path"
        }
        return 1;
    } else if (strcmp(parsed[0], "echo") == 0) {
        executaEcho(parsed); // Chama a função executaEcho para o comando 'echo'
        return 1; // Indica que foi um comando embutido
    }
    return 0;
}

int existeDirecionador(char* str, char** comandosDirecionador) {
    int i = 0; // Inicializa o contador para índices dos comandosDirecionador
    char *token = strtok(str, ">"); // Divide a string str em tokens usando ">" como delimitador
    while (token != NULL) { // Enquanto houver tokens
        comandosDirecionador[i++] = token; // Armazena o token atual no array comandosDirecionador e incrementa o contador
        token = strtok(NULL, ">"); // Obtém o próximo token da string
    }
    comandosDirecionador[i] = NULL; // Adiciona um último elemento nulo ao final do array comandosDirecionador
    return i > 1; // Retorna verdadeiro se houver mais de um token (indicando a presença de um direcionador ">")
}


int existePipe(char* str, char** comandosPipe) {
    for (int i = 0; i < 2; i++) { // Loop para dividir a string em dois comandos separados pelo caractere '&'
        comandosPipe[i] = strsep(&str, "&"); // Divide a string em um comando e armazena em comandosPipe[i]
        if (comandosPipe[i] == NULL) // Verifica se não há mais comandos
            break; // Sai do loop se não houver mais comandos
    }
    return comandosPipe[1] != NULL; // Retorna verdadeiro se houver um segundo comando, indicando a presença de um pipe
}


int processarString(char* comandosDeEntrada, char** argumentos, char** argumentosPipe, char** arquivos, char** arquivo_pipe) {
    char* comandosPipe[2]; // Array para armazenar os comandos em caso de pipe
    int piped = existePipe(comandosDeEntrada, comandosPipe); // Verifica se há um pipe na entrada e armazena os comandos separados no array comandosPipe

    int direcionador_duplo = 0; // Variável para verificar a existência de um redirecionador
    int direcionador_duplo_direita = 0; // Variável para verificar a existência de um redirecionador para a direita
    char* argumentosRedirecionador[2]; // Arrays para armazenar os argumentos após o redirecionamento
    char* argumentosRedirecionador_direita[2]; // Arrays para armazenar os argumentos após o redirecionamento para a direita
    // Se houver um pipe
    if (piped) {
        direcionador_duplo = existeDirecionador(comandosPipe[0], argumentosRedirecionador); // Verifica se há redirecionador na parte esquerda do pipe
        direcionador_duplo_direita = existeDirecionador(comandosPipe[1], argumentosRedirecionador_direita); // Verifica se há redirecionador na parte direita do pipe
        // Se houver redirecionador na parte esquerda do pipe
        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos); // Remove os espaços dos argumentos redirecionados e os armazena em arquivos
        }
        // Se houver redirecionador na parte direita do pipe
        if(direcionador_duplo_direita > 0){
            retirarEspaco(argumentosRedirecionador_direita[1], arquivos); // Remove os espaços dos argumentos redirecionados e os armazena em arquivos
        }
        retirarEspaco(comandosPipe[0], argumentos); // Remove os espaços dos argumentos do comando na parte esquerda do pipe e os armazena em argumentos
        retirarEspaco(comandosPipe[1], argumentosPipe); // Remove os espaços dos argumentos do comando na parte direita do pipe e os armazena em argumentosPipe
    } else {
        direcionador_duplo = existeDirecionador(comandosDeEntrada, argumentosRedirecionador); // Se não houver pipe, verifica se há redirecionador na entrada
        // Se houver redirecionador na entrada
        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos); // Remove os espaços dos argumentos redirecionados e os armazena em arquivos
        }
        retirarEspaco(comandosDeEntrada, argumentos); // Remove os espaços dos argumentos do comando na entrada e os armazena em argumentos
    }
    // Verifica se os argumentos são comandos embutidos
    if (builtinComandos(argumentos))
        return 0; // Retorna 0 se for um comando embutido
    else
        // Retorna 1 se não houver pipe, 2 se houver pipe, 3 se houver pipe e redirecionador
        return 1 + piped + direcionador_duplo;
}

void printDir() {
    // Obtém o nome do host
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("Erro ao obter o nome do host");
        return;
    }

    // Obtém o nome do usuário logado
    username = getenv("USER");
    if (username == NULL) {
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            username = pw->pw_name;
        } else {
            perror("Erro ao obter o nome do usuário");
            return;
        }
    }

    // Imprime o usuário, host e diretório atual com as cores especificadas
    printf("\n" ANSI_COLOR_YELLOW "%s@%s:" ANSI_COLOR_BLUE "◆%s > " ANSI_COLOR_RESET, username, hostname, current_dir);
}

void redirecionar_saida(char** parsed, char *nome_arquivo, int write_end) {
    pid_t cpid; // Variável para o ID do processo filho
    int rtrnstatus; // Variável para o status de retorno do processo filho
    int saida; // Descritor de arquivo para a saída redirecionada
    int save_out; // Descritor de arquivo para salvar a saída padrão

    // Abre o arquivo de saída para escrita, com ou sem truncamento, dependendo de write_end
    if(write_end == 1) {
        saida = open(nome_arquivo, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
        if (-1 == saida){
            perror(nome_arquivo);
            return;
        }
    } else {
        saida = open(nome_arquivo, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (-1 == saida){
            perror(nome_arquivo);
            return;
        }
    }

    save_out = dup(fileno(stdout)); // Salva a saída padrão

    // Redireciona a saída padrão para o arquivo especificado
    if (-1 == dup2(saida, fileno(stdout))){
        perror("Cannot redirect stdout");
        return;
    }

    cpid = fork(); // Cria um processo filho

    if (cpid < 0){ // Se houver erro no fork
        perror("Fork failed");
        return;
    } else if (cpid == 0){ // Processo filho
        execvp(parsed[0], parsed); // Executa o comando com os argumentos fornecidos
    } else { // Processo pai
        waitpid(cpid, &rtrnstatus, 0); // Aguarda o término do processo filho
        dup2(save_out, fileno(stdout)); // Restaura a saída padrão
        close(save_out); // Fecha o descritor de arquivo salvo
        close(saida); // Fecha o descritor de arquivo da saída redirecionada

        // Verifica se o processo filho terminou normalmente
        if (WIFEXITED(rtrnstatus)){
            return; // Retorna se o processo filho terminou normalmente
        } else {
            printf( ANSI_COLOR_RED "Child did not terminate with exit\n" ANSI_COLOR_RESET); // Exibe mensagem se o processo filho não terminou normalmente
        }
    }
}


char* encontrar_comando(const char* comando) {
    static char caminho_completo[MAX_PATH_LENGTH]; 
    // Itera sobre todos os caminhos armazenados
    for (int i = 0; i < path_count; i++) {
        // Constrói o caminho completo do comando concatenando o caminho atual e o comando
        snprintf(caminho_completo, sizeof(caminho_completo), "%s/%s", paths[i], comando);
        // Verifica se o caminho completo do comando é acessível e executável
        if (access(caminho_completo, X_OK) == 0) {
            return caminho_completo; // Retorna o caminho completo se o comando for encontrado
        }
    }
    return NULL; // Retorna NULL se o comando não for encontrado em nenhum dos caminhos
}
void executaArgsUnitarios(char** parsed, char* nome_arquivo) {
    pid_t pid = fork(); // Cria um processo filho

    if (pid == -1) { // Verifica se houve erro no fork
        perror("Erro no fork"); // Exibe mensagem de erro
        return;
    } else if (pid == 0) { // Processo filho
        // Se o nome do arquivo de saída for especificado
        if (nome_arquivo != NULL) {
            // Abre o arquivo para escrita
            int fd = open(nome_arquivo, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open"); // Exibe mensagem de erro, caso ocorra
                exit(EXIT_FAILURE); // Sai do processo filho em caso de falha
            }
            // Redireciona a saída padrão para o arquivo
            dup2(fd, STDOUT_FILENO);
            close(fd); // Fecha o descritor de arquivo
        }
        // Encontra o caminho completo do comando
        char* caminho_completo = encontrar_comando(parsed[0]);
        if (caminho_completo != NULL) {
            // Executa o comando com os argumentos fornecidos
            execv(caminho_completo, parsed);
            // Se o execv retornar, algo deu errado
            perror("Erro ao executar comando");
            exit(EXIT_FAILURE);
        } else {
            // Exibe mensagem de erro se o comando não for encontrado
            fprintf(stderr, ANSI_COLOR_RED "Comando não encontrado: %s\n" ANSI_COLOR_RESET, parsed[0]);
            exit(EXIT_FAILURE); // Sai do processo filho em caso de falha
        }
    } else { // Processo pai
        int status;
        // Espera pelo término do processo filho
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char *argv[]) {
    
    char entradaUsuario[MAXCOM]; // String para armazenar a entrada do usuário
    char *argumentos[MAXLIST]; // Array de strings para armazenar os argumentos de um comando
    char *argumentosPipe[MAXLIST]; // Array de strings para armazenar os argumentos de um comando em um pipe
    char *arquivos[MAXLIST]; // Array de strings para armazenar os nomes de arquivos
    char *arquivo_pipe[MAXLIST]; // Array de strings para armazenar os nomes de arquivos em um pipe
    int execFlag = 0; // Flag para indicar se o comando deve ser executado
    
    init_current_dir(); // Inicializa o diretório atual

    // Se nenhum argumento de linha de comando for fornecido, execute normalmente
    if (argc == 1) {
        clear(); // Limpa a tela do console

        while (1) {
            printDir(); // Exibe o diretório atual
            char* linha_comando = read_line(); // Lê a linha de comando do usuário
            execFlag = processarString(linha_comando, argumentos, argumentosPipe, arquivos, arquivo_pipe); // Processa a linha de comando
            
            if (execFlag == 1) {
                executaArgsUnitarios(argumentos, NULL); // Executa um comando unitário
            } else if (execFlag == 2) {
                int write_end = 0;
                if(strcmp(argumentosPipe[0],">>") == 0){
                    write_end = 1;
                }
                redirecionar_saida(argumentos, arquivos[0], write_end); // Redireciona a saída do comando
            }
            // Adicione outras condições para lidar com comandos encadeados ou redirecionamentos, se necessário
            free(linha_comando); // Libera a memória alocada para a linha de comando
        }
    } else if (argc == 2) { // Se um argumento de linha de comando for fornecido
        FILE *batchFile = fopen(argv[1], "r"); // Abre o arquivo batch para leitura
        if (batchFile == NULL) { // Verifica se houve erro na abertura do arquivo
            perror("Erro ao abrir o arquivo batch");
            return 1;
        }

        while (fgets(entradaUsuario, MAXCOM, batchFile) != NULL) { // Lê cada linha do arquivo batch
            entradaUsuario[strcspn(entradaUsuario, "\n")] = 0; // Remove o caractere de nova linha
            execFlag = processarString(entradaUsuario, argumentos, argumentosPipe, arquivos, arquivo_pipe); // Processa a linha do arquivo batch
            
            if (execFlag == 1) {
                executaArgsUnitarios(argumentos, NULL); // Executa um comando unitário
            } else if (execFlag == 2) {
                int write_end = 0;
                if(strcmp(argumentosPipe[0],">>") == 0){
                    write_end = 1;
                }
                redirecionar_saida(argumentos, arquivos[0], write_end); // Redireciona a saída do comando
            }
            // Adicione outras condições para lidar com comandos encadeados ou redirecionamentos, se necessário
        }

        fclose(batchFile); // Fecha o arquivo batch após a leitura
    } else { // Se mais de um argumento de linha de comando for fornecido
        printf( ANSI_COLOR_GREEN "Uso: %s [arquivo_batch]\n" ANSI_COLOR_RESET, argv[0]); // Exibe mensagem de uso correto
        return 1;
    }

    return 0;
}