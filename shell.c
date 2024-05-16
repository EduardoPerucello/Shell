#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h> // para usar getpwuid


#define MAXCOM 1000 
#define MAXLIST 100
#define clear() printf("\033[H\033[J")
#define BUFSIZE 1024
#define MAX_PATHS 100
#define MAX_PATH_LENGTH 1024

// Vetor global para armazenar caminhos
char* paths[MAX_PATHS];
int path_count = 0;

// Atualiza o diretório atual global
char current_dir[BUFSIZE];
char path_catls[BUFSIZE];

// Inicialização do diretório atual global com /home/<usuário_logado>
void init_current_dir() {
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("Erro ao obter o diretório atual");
        exit(EXIT_FAILURE);
    }
}

// Função para ler a linha de comando
char *read_line(void)
{
    int bufsize = BUFSIZE; // Tamanho inicial do buffer
    int position = 0; // Posição atual dentro do buffer
    char *buffer = malloc(sizeof(char) * bufsize); // Aloca memória para o buffer
    int c;

    // Verifica se a alocação de memória foi bem sucedida
    if (!buffer)
    {
        fprintf(stderr, "Allocation error\n");
        exit(EXIT_FAILURE);
    }

    // Loop infinito para ler os caracteres de entrada
    while (1)
    {
        c = getchar(); // Lê o próximo caractere da entrada do usuário

        // Verifica o fim da linha ou do arquivo
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0'; // Adiciona um caractere nulo ao final do buffer
            return buffer; // Retorna o buffer
        }
        else
        {
            buffer[position] = c; // Armazena o caractere lido no buffer na posição atual
        }

        position++; // Incrementa a posição

        // Verifica se é necessário realocar o buffer para acomodar mais caracteres
        if (position >= bufsize)
        {
            bufsize += BUFSIZE; // Aumenta o tamanho do buffer
            buffer = realloc(buffer, bufsize); // Realoca o buffer com o novo tamanho

            // Verifica se a realocação foi bem sucedida
            if (!buffer)
            {
                fprintf(stderr, "Allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

//RETORNAR OS COMANDOS SEPARADOS POR ESPAÇO
void retirarEspaco(char* str, char** comandos)
{
    int i = 0; // Variável para rastrear a posição atual nos comandos
    char *token = strtok(str, " "); // Divide a string em tokens usando o delimitador "espaço"

    // Loop para iterar sobre os tokens enquanto houver tokens disponíveis
    while (token != NULL)
    {
        comandos[i++] = token; // Armazena o token atual na matriz de comandos e incrementa o índice
        token = strtok(NULL, " "); // Obtém o próximo token da string
    }
    
    comandos[i] = NULL; // Adiciona NULL ao final da lista de comandos para indicar o término
}

void add_path(const char* path) {
    if (access(path, F_OK) == -1) { // Verifica se o caminho existe
        fprintf(stderr, "Caminho não existe: %s\n", path);
        return;
    }

    if (path_count < MAX_PATHS) {
        paths[path_count] = strdup(path); // Aloca memória e copia o caminho
        path_count++;
    } else {
        fprintf(stderr, "Limite de caminhos atingido\n");
    }
}

void remove_path(const char* path) {
    for (int i = 0; i < path_count; i++) {
        if (strcmp(paths[i], path) == 0) {
            free(paths[i]); // Libera a memória alocada
            for (int j = i; j < path_count - 1; j++) {
                paths[j] = paths[j + 1];
            }
            path_count--;
            return;
        }
    }
    fprintf(stderr, "Caminho não encontrado: %s\n", path);
}

void list_paths() {
    for (int i = 0; i < path_count; i++) {
        printf("%s\n", paths[i]);
    }
}

int builtinComandos(char** parsed) {
    if (strcmp(parsed[0], "cd") == 0) {
        if (parsed[1] == NULL || strcmp(parsed[1], "~") == 0) {
            struct passwd *pw = getpwuid(getuid());
            if (pw == NULL) {
                perror("getpwuid");
                return 1;
            }
            if (chdir(pw->pw_dir) != 0) {
                perror("chdir");
                return 1;
            }
        } else if (strcmp(parsed[1], "..") == 0) {
            if (chdir("..") != 0) {
                perror("chdir");
                return 1;
            }
        } else {
            if (chdir(parsed[1]) != 0) {
                perror("chdir");
                return 1;
            }
        }
        init_current_dir();
        return 1;
    } else if (strcmp(parsed[0], "exit") == 0) {
        printf("\nGoodbye\n");
        exit(EXIT_SUCCESS);
    } else if (strcmp(parsed[0], "path") == 0) {
        if (parsed[1] == NULL) {
            list_paths();
        } else if (strcmp(parsed[1], "add") == 0 && parsed[2] != NULL) {
            for (int i = 2; parsed[i] != NULL; i++) {
                add_path(parsed[i]);
            }
        } else if (strcmp(parsed[1], "remove") == 0 && parsed[2] != NULL) {
            for (int i = 2; parsed[i] != NULL; i++) {
                remove_path(parsed[i]);
            }
        } else {
            fprintf(stderr, "Uso: path [add|remove] [caminho1 caminho2 ...]\n");
        }
        return 1;
    }
    return 0;
}

//VERIFICA SE EXISTE DIRECIONADOR PARA ARQUIVO
int existeDirecionador(char* str, char** comandosDirecionador)
{
    int i = 0; // Variável para rastrear a posição atual nos comandosDirecionador
    char *token = strtok(str, ">"); // Divide a string usando ">" como delimitador

    // Loop para iterar sobre os tokens enquanto houver tokens disponíveis
    while (token != NULL)
    {
        comandosDirecionador[i++] = token; // Armazena o token atual no array de comandosDirecionador e incrementa o índice
        token = strtok(NULL, ">"); // Obtém o próximo token da string
    }
    
    comandosDirecionador[i] = NULL; // Adiciona NULL ao final da lista de comandosDirecionador para indicar o término

    // Retorna 1 se houver mais de um token (indicando direcionador duplo), caso contrário retorna 0
    return i > 1;
}

//VERIFICA SE EXISTE PIPE
int existePipe(char* str, char** comandosPipe)
{
    // Loop para dividir a string em dois comandos separados pelo pipe
    for (int i = 0; i < 2; i++)
    {
        // Divide a string em comandos separados pelo pipe '&' e armazena cada parte em comandosPipe[i]
        comandosPipe[i] = strsep(&str, "&");

        // Se não houver mais comandos, sai do loop
        if (comandosPipe[i] == NULL)
            break;
    }

    // Se o segundo comando for NULL, significa que não há pipe
    if (comandosPipe[1] == NULL)
        return 0; // Retorna 0 para indicar que nenhum pipe foi encontrado
    else
        return 1; // Retorna 1 para indicar que um pipe foi encontrado
}

//PROCESSA STRING E QUEBRA EM COMANDOS
int processarString(char* comandosDeEntrada, char** argumentos, char** argumentosPipe, char** arquivos, char** arquivo_pipe)
{
    char* comandosPipe[2]; // Array para armazenar os comandos divididos por pipe
    int piped = 0; // Flag para indicar se há pipe

    int direcionador_duplo = 0; // Flag para indicar se há redirecionamento duplo
    int direcionador_duplo_direita = 0; // Flag para indicar se há redirecionamento duplo à direita
    char* argumentosRedirecionador[2]; // Array para armazenar os argumentos divididos por redirecionador duplo
    char* argumentosRedirecionador_direita[2]; // Array para armazenar os argumentos divididos por redirecionador duplo à direita

    int existeRedirecionador;

    // Verifica se há pipe na string de comando
    piped = existePipe(comandosDeEntrada, comandosPipe);

    if (piped)
    {
        // Se houver pipe, divide os comandos em dois lados (esquerda e direita)
        direcionador_duplo = existeDirecionador(comandosPipe[0], argumentosRedirecionador);
        direcionador_duplo_direita = existeDirecionador(comandosPipe[1], argumentosRedirecionador_direita);

        // Se houver redirecionador duplo à esquerda, remove os espaços em branco e armazena o arquivo
        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos);
        }

        // Se houver redirecionador duplo à direita, remove os espaços em branco e armazena o arquivo
        if(direcionador_duplo_direita > 0){
            retirarEspaco(argumentosRedirecionador_direita[1], arquivos);
        }

        // Divide os argumentos dos comandos em cada lado do pipe
        retirarEspaco(comandosPipe[0], argumentos);
        retirarEspaco(comandosPipe[1], argumentosPipe);
    } else{
        // Se não houver pipe, verifica se há redirecionador duplo
        direcionador_duplo = existeDirecionador(comandosDeEntrada, argumentosRedirecionador);

        // Se houver redirecionador duplo, remove os espaços em branco e armazena o arquivo
        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos);
        }
        
        // Se não houver pipe e apenas argumentos no lado esquerdo, divide os argumentos
        // da string de comando
        retirarEspaco(comandosDeEntrada, argumentos);
    }

    // Resetando a flag direcionador_duplo para 0 se for 1
    if(direcionador_duplo == 1) direcionador_duplo = 0;

    // Verifica se os argumentos representam um comando interno (built-in)
    if (builtinComandos(argumentos))
        return 0; // Retorna 0 se for um comando interno
    else
        return 1 + piped + direcionador_duplo; // Retorna 1 + piped + direcionador_duplo se for um comando externo
}

//MOSTRA O DIRETORIO RAIZ
void printDir()
{
    printf("\n%s > ", current_dir); // Certifique-se de que current_dir está sendo usado corretamente aqui
}

//REDICIONAR A SAIDA
void redirecionar_saida(char** parsed, char *nome_arquivo, int write_end)
{
    pid_t cpid; // Identificador do processo filho
    int rtrnstatus; // Status de retorno do processo filho
    int saida; // Descritor de arquivo para o arquivo de saída
    int save_out; // Descritor de arquivo para salvar a saída padrão

    // Verifica se é para escrever no final do arquivo ou truncá-lo
    if(write_end == 1)
    {
        // Abre o arquivo de saída em modo de escrita, adicionando no final do arquivo
        saida = open(nome_arquivo, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
        // Verifica se houve erro ao abrir o arquivo
        if (-1 == saida){
            perror(nome_arquivo); // Exibe uma mensagem de erro
            return; // Retorna da função
        }
    }
    else
    {
        // Abre o arquivo de saída em modo de escrita, truncando-o se já existir ou criando-o se não existir
        saida = open(nome_arquivo, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        // Verifica se houve erro ao abrir o arquivo
        if (-1 == saida){
            perror(nome_arquivo); // Exibe uma mensagem de erro
            return; // Retorna da função
        }
    }

    // Salva a saída padrão atual para restaurá-la posteriormente
    save_out = dup(fileno(stdout));

    // Redireciona a saída padrão para o arquivo de saída
    if (-1 == dup2(saida, fileno(stdout))){
        perror("Cannot redirect stdout"); // Exibe uma mensagem de erro se houver falha ao redirecionar a saída padrão
        return; // Retorna da função
    }

    // Cria um processo filho
    cpid = fork();

    // Verifica se houve erro ao criar o processo filho
    if (cpid < 0){
        perror("Fork failed"); // Exibe uma mensagem de erro
        return; // Retorna da função
    }
    // Código a ser executado pelo processo filho
    else if (cpid == 0){
        execvp(parsed[0], parsed); // Executa o comando com os argumentos fornecidos
    }
    // Código a ser executado pelo processo pai
    else{
        waitpid(cpid, &rtrnstatus, 0); // Espera pelo término do processo filho
        dup2(save_out, fileno(stdout)); // Restaura a saída padrão
        close(save_out); // Fecha o descritor de arquivo salvando a saída padrão
        close(saida); // Fecha o descritor de arquivo do arquivo de saída

        // Verifica se o processo filho terminou normalmente
        if (WIFEXITED(rtrnstatus)){
            return; // Retorna da função
        }
        else{
            printf("Child did not terminate with exit\n"); // Exibe uma mensagem de erro se o processo filho não terminou normalmente
        }
    }
}

//FUNÇÃO PARA EXECUTAR COMANDOS UNITÁRIOS
void executaArgsUnitarios(char* comandosDeEntrada, char** parsed, char** arquivos, int direcionador_duplo)
{
    pid_t pid = fork(); // Cria um processo filho

    // Verifica se houve erro ao criar o processo filho
    if (pid == -1)
    {
        printf("\nO processo de forking falhou.."); // Exibe uma mensagem de erro
        return; // Retorna da função
    }
    // Código a ser executado pelo processo filho
    else if (pid == 0)
    {
        // Verifica se o comando é 'ls' ou 'cat'
        if (strcmp(parsed[0], "ls") == 0 || strcmp(parsed[0], "cat") == 0)
        {
            // Se sim, executa 'ls' ou 'cat' com os argumentos fornecidos
            char cwd[1024]; // Array para armazenar o diretório atual
            if (getcwd(cwd, sizeof(cwd)) != NULL) { // Obtém o diretório atual
                char command_path[2048]; // Array para armazenar o caminho completo do comando
                sprintf(command_path, "%s/%s", path_catls, parsed[0]); // Formata o caminho do comando
                parsed[0] = command_path; // Atualiza o caminho do comando nos argumentos
                execvp(parsed[0], parsed); // Executa o comando com os argumentos fornecidos
            } else {
                perror("getcwd() error"); // Exibe uma mensagem de erro se não for possível obter o diretório atual
                exit(EXIT_FAILURE); // Sai do processo filho com falha
            }
        }
        else
        {
            // Se não for 'ls' ou 'cat', executa o comando normalmente
            if (arquivos[0] != NULL) // Verifica se há redirecionamento de saída
            {
                redirecionar_saida(parsed, arquivos[0], direcionador_duplo); // Redireciona a saída para o arquivo especificado
            }
            else
            {
                int ret = execvp(parsed[0], parsed); // Executa o comando com os argumentos fornecidos
                if (ret < 0) // Verifica se houve erro ao executar o comando
                {
                    printf("\nEsse comando não pode ser executado.."); // Exibe uma mensagem de erro
                }
            }
        }
        exit(0); // Sai do processo filho
    }
    // Código a ser executado pelo processo pai
    else
    {
        wait(NULL); // Aguarda o processo filho finalizar
        return; // Retorna da função
    }
}

// FUNÇÃO PARA LIDAR COM OS COMANDOS DENTRO DE UM PIPE
void executaArgsEmPipe(char** parsed, char** parsedpipe, char** arquivo_esquerda, char** arquivo_direita, int direcionador_duplo)
{
    int ret; // Variável para armazenar o valor de retorno de funções
    int pipefd[2]; // Array para armazenar os descritores do pipe
    pid_t processo_esquerda, processo_direita; // Identificadores dos processos

    // Cria o pipe
    if (pipe(pipefd) < 0){
        printf("A inicialização do pipe falhou"); // Exibe uma mensagem de erro se a criação do pipe falhar
        return; // Retorna da função
    }

    // Cria um processo filho para o lado esquerdo do pipeline
    processo_esquerda = fork();

    // Verifica se houve erro ao criar o processo filho
    if (processo_esquerda < 0){
        printf("\nO processo de forking falhou.."); // Exibe uma mensagem de erro
        return; // Retorna da função
    }

    // Código a ser executado pelo processo filho (lado esquerdo)
    if (processo_esquerda == 0)
    {
        // Fecha o descritor de leitura do pipe
        close(pipefd[0]);
        // Redireciona a saída padrão para o extremo de escrita do pipe
        dup2(pipefd[1], STDOUT_FILENO);
        // Fecha o extremo de escrita do pipe
        close(pipefd[1]);

        // Executa o comando do lado esquerdo do pipeline
        if (execvp(parsed[0], parsed) < 0){
            printf("\nCould not execute command 1.."); // Exibe uma mensagem de erro se a execução do comando falhar
            exit(0); // Sai do processo filho
        }
    }
    else
    {
        // Código a ser executado pelo processo pai

        // Verifica se o próximo comando é separado por '&' (em segundo plano)
        if (parsedpipe[0] != NULL && strcmp(parsedpipe[0], "&") == 0)
        {
            printf("Comando da esquerda em segundo plano\n"); // Exibe uma mensagem indicando que o comando da esquerda está sendo executado em segundo plano
        }
        else
        {
            wait(NULL); // Espera o processo da esquerda terminar
        }

        // Cria um processo filho para o lado direito do pipeline
        processo_direita = fork();

        // Verifica se houve erro ao criar o processo filho
        if (processo_direita < 0){
            printf("\nO processo de forking falhou.."); // Exibe uma mensagem de erro
            return; // Retorna da função
        }

        // Código a ser executado pelo processo filho (lado direito)
        if (processo_direita == 0)
        {
            // Fecha o descritor de escrita do pipe
            close(pipefd[1]);
            // Redireciona a entrada padrão para o extremo de leitura do pipe
            dup2(pipefd[0], STDIN_FILENO);
            // Fecha o extremo de leitura do pipe
            close(pipefd[0]);

            // Executa o comando do lado direito do pipeline
            if (execvp(parsedpipe[0], parsedpipe) < 0){
                printf("\nCould not execute command 2.."); // Exibe uma mensagem de erro se a execução do comando falhar
                exit(0); // Sai do processo filho
            }
        }
        else
        {
            wait(NULL); // Espera o processo da direita terminar
        }
    }
}

int main(int argc, char* argv[]) {
    // Compila os programas 'ls' e 'cat'
    system("gcc -o ls ls.c");
    system("gcc -o cat cat.c");

    // Inicializa o diretório atual e o armazena em 'path_catls'
    init_current_dir();

    // Inicializa o diretório atual em uma cópia de 'path_catls'
    strcpy(path_catls, current_dir);

    // Limpa a tela
    clear();

    char *comandosDeEntrada, *argumentos[MAXLIST];
    char* argumentosPipe[MAXLIST];
    char* nomeArquivo[2];
    char* arquivo_pipe[2];
    int pipeOuUnitario = 0;

    FILE* batchFile = NULL;
    if (argc > 1) {
        // Se um arquivo batch é passado como argumento, abre o arquivo
        batchFile = fopen(argv[1], "r");
        if (batchFile == NULL) {
            perror("Erro ao abrir o arquivo batch");
            return 1;
        }
    }

    // Loop principal para a execução contínua do shell
    while (1) {
        // Imprime o diretório atual
        printDir();

        // Reinicia os arrays para armazenar comandos e argumentos
        nomeArquivo[0] = NULL;
        arquivo_pipe[0] = NULL;

        if (batchFile) {
            // Lê a linha de comando do arquivo batch
            size_t len = 0;
            if (getline(&comandosDeEntrada, &len, batchFile) == -1) {
                // Fim do arquivo, fecha o arquivo batch e sai do loop
                fclose(batchFile);
                break;
            }
        } else {
            // Lê a linha de comando do stdin
            comandosDeEntrada = read_line();
        }

        // Processa a linha de comando e determina se é um comando único, um pipeline ou um redirecionamento
        pipeOuUnitario = processarString(comandosDeEntrada, argumentos, argumentosPipe, nomeArquivo, arquivo_pipe);

        // Executa o comando único se não houver pipeline ou redirecionamento
        if (pipeOuUnitario == 1) {
            executaArgsUnitarios(comandosDeEntrada, argumentos, nomeArquivo, 0);
        }

        // Executa o pipeline se houver pipeline
        if (pipeOuUnitario == 2) {
            executaArgsEmPipe(argumentos, argumentosPipe, nomeArquivo, arquivo_pipe, 0);
        }

        // Executa o comando único com redirecionamento de saída se houver redirecionamento
        if (pipeOuUnitario == 3) {
            executaArgsUnitarios(comandosDeEntrada, argumentos, nomeArquivo, 1);
        }

        // Executa o pipeline com redirecionamento de saída se houver redirecionamento
        if (pipeOuUnitario == 4) {
            executaArgsEmPipe(argumentos, argumentosPipe, nomeArquivo, arquivo_pipe, 1);
        }

        // Libera a memória alocada para a linha de comando após a execução
        free(comandosDeEntrada);
    }

    return 0; // Retorna 0 para indicar a conclusão bem-sucedida do programa
}

