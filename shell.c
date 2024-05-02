#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAXCOM 1000 
#define MAXLIST 100
#define clear() printf("\033[H\033[J")
#define BUFSIZE 1024


//LEITURA DA LINHA ATÉ O ENTER
char *read_line(void)
{
    int bufsize = BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        c = getchar();

        //VERIFICA FIM DA LINHA
        if (c == EOF || c == '\n')
        {
            buffer[position] = '\0';
            return buffer;
        }else{
            buffer[position] = c;
        }

        position++;

        //SE O TAMANHO EXCEDE O BUFFER ELE REALOCA MAIS ESPAÇO
        if (position >= bufsize)
        {
            bufsize += BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer)
            {
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

    }
    
}

//RETORNAR OS COMANDOS SEPARADOS POR ESPAÇO
void retirarEspaco(char* str, char** comandos)
{
    int i = 0;
    char *token = strtok(str, " ");
    while (token != NULL)
    {
        comandos[i++] = token;
        token = strtok(NULL, " ");
    }
    comandos[i] = NULL; // Adiciona NULL para indicar o fim da lista de comandos
}

//COMANDO EXIT E CD
int builtinComandos(char** parsed)
{
    int size = 2, argumentosIndentificador = 0;
    char* comandosDeEspecificos[size];

    comandosDeEspecificos[0] = "exit";
    comandosDeEspecificos[1] = "cd";

    for (int i = 0; i < size; i++)
    {
        //CASO O COMANDO CORRESPONDA A ALGUM DOS ESPECIFICOS, GUARDAMOS A POSIÇÃO PARA O SWITCH/CASE
        if (strcmp(parsed[0], comandosDeEspecificos[i]) == 0){
            argumentosIndentificador = i + 1;
            break;
        }
    }

    switch (argumentosIndentificador){
    case 1:
        printf("\nGoodbye\n");
        exit(0);
    case 2:
        chdir(parsed[1]);
        return 1;
    default:
        break;
    }
    return 0;
}

//VERIFICA SE EXISTE DIRECIONADOR PARA ARQUIVO
int existeDirecionador(char* str, char** comandosDirecionador)
{
    int i = 0;
    char *token = strtok(str, ">");
    while (token != NULL)
    {
        comandosDirecionador[i++] = token;
        token = strtok(NULL, ">");
    }
    comandosDirecionador[i] = NULL; // Adiciona NULL para indicar o fim da lista de comandos
    return i > 1; // Retorna 1 se houver mais de um token (indicando direcionador duplo)
}

//VERIFICA SE EXISTE PIPE
int existePipe(char* str, char** comandosPipe)
{
    for (int i = 0; i < 2; i++)
    {
        //GUARDA COMANDOS A ESQUERDA DO PIPE EM comandosPipe[0] E DIREITA EM comandosPipe[1]
        comandosPipe[i] = strsep(&str, "|");
        if (comandosPipe[i] == NULL)
            break;
    }

    if (comandosPipe[1] == NULL)
        return 0; //NENHUM PIPE ENCONTRADO
    else
        return 1;
}

//PROCESSA STRING E QUEBRA EM COMANDOS
int processarString(char* comandosDeEntrada, char** argumentos, char** argumentosPipe, char** arquivos, char** arquivo_pipe)
{
    char* comandosPipe[2];
    int piped = 0;

    int direcionador_duplo = 0;
    int direcionador_duplo_direita = 0;
    char* argumentosRedirecionador[2];
    char* argumentosRedirecionador_direita[2];

    int existeRedirecionador;

    //ESCREVE EM comandosPiped O LADO ESQUERDO E DIREITO DO PIPE
    piped = existePipe(comandosDeEntrada, comandosPipe);

    if (piped)
    {
        //ARMAZENA A RESPOSTA EM UMA LISTA DE STRINGS OS COMANDOS (ESQUERDA EM argumenos E DIREITA EM argumentosPipe)
        direcionador_duplo = existeDirecionador(comandosPipe[0], argumentosRedirecionador);
        direcionador_duplo_direita = existeDirecionador(comandosPipe[1], argumentosRedirecionador_direita);

        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos);
        }

        if(direcionador_duplo_direita > 0){
            retirarEspaco(argumentosRedirecionador_direita[1], arquivos);
        }

        retirarEspaco(comandosPipe[0], argumentos);
        retirarEspaco(comandosPipe[1], argumentosPipe);
    } else{
        direcionador_duplo = existeDirecionador(comandosDeEntrada, argumentosRedirecionador);
        if(direcionador_duplo > 0){
            retirarEspaco(argumentosRedirecionador[1], arquivos);
        }
        
        //CASO ONDE NÃO EXISTE PIPE E APENAS ARGUMENTOS NO LADO ESQUERDO
        retirarEspaco(comandosDeEntrada, argumentos);
    }

    if(direcionador_duplo == 1) direcionador_duplo = 0;

    if (builtinComandos(argumentos))
        return 0;
    else
        return 1 + piped + direcionador_duplo;
}

//MOSTRA O DIRETORIO RAIZ
void printDir()
{
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("\n%s > ", cwd);

}

//REDICIONAR A SAIDA
void redirecionar_saida(char** parsed, char *nome_arquivo, int write_end)
{
    pid_t cpid;
    int rtrnstatus;
    int saida;
    int save_out;

    if(write_end ==1)
    {
        saida = open(nome_arquivo, O_APPEND | O_WRONLY, S_IRUSR | S_IWUSR);
        if (-1 == saida){
            perror(nome_arquivo);
            return;
        }
    }else{
        saida = open(nome_arquivo, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
         if (-1 == saida){
            perror(nome_arquivo);
            return;
        }
    }

    save_out = dup(fileno(stdout));

    if (-1 == dup2(saida, fileno(stdout))){
        perror("Cannot redirect stdout");
        return;
    }

    cpid = fork();

    if (cpid < 0){
        perror("Fork failed");
        return;
    }else if (cpid == 0){
        execvp(parsed[0], parsed);
    }else{
        waitpid(cpid, &rtrnstatus, 0);
        dup2(save_out, fileno(stdout));
        close(save_out);
        close(saida);

        if (WIFEXITED(rtrnstatus)){
            return;
        }else{
            printf("Child did not terminate with exit\n");
        }
    }
}

//FUNÇÃO PARA EXECUTAR COMANDOS UNITARIOS
void executaArgsUnitarios(char* comandosDeEntrada, char** parsed, char** arquivos, int direcionador_duplo)
{
    pid_t pid = fork();

    if (pid == -1)
    {
        printf("\nO processo de forking falhou..");
        return;
    }else if (pid == 0){
        if(arquivos[0] != NULL){
            redirecionar_saida(parsed, arquivos[0], direcionador_duplo);
        }else{
            int ret = execvp(parsed[0], parsed);
            if(ret < 0){
                printf("\nEsse comando não pode ser executado..");
            }
        }
        exit(0);
    }else{
        //AGUARDANDO O PROCESSO FINALIZAR
        wait(NULL);
        return;
    }
}

//FUNÇÃO PARA LIDAR COM OS COMANDOS DENTRO DE UM PIPE
void executaArgsEmPipe(char** parsed, char** parsedpipe, char** arquivo_esquerda, char** arquivo_direita, int direcionador_duplo)
{
    int ret;
    int pipefd[2];
    pid_t processo_esquerda, porcesso_direita;

    if (pipe(pipefd) < 0){
        printf("A inicialização do pipe falhou");
        return;
    }

    processo_esquerda = fork();

    if (porcesso_esquerda < 0){
        printf("\nO processo de forking falhou..")
        return;
    }

    if (processo_esquerda == 0)
    {
        //PROCESSO FILHO 1 EM EXECUÇÃO
        //ESCRITA NO FINAL DO DESCRITOR
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        if (execvp(parsed[0], parsed) < 0){
            printf("\nCould not execute command 1..");
            exit(0);
        }
    }else{
        //PAI EM EXECUÇÃO
        processo_direita = fork();
        if (processo_direita < 0){
            printf("\nO processo de forking falhou..")
            return;
        }

        //PROCESSO FILHO 2 EM EXECUÇÃO
        //LEITURA DO FINAL DO LEITOR
        if (processo_direita == 0)
        {
            close(pipefd[1]);
            dup2(pipefd[0], STDIN_FILENO);
            close(pipefd[0]);

            if (execvp(parsed[0], parsed) < 0){
                printf("\nCould not execute command 2..");
                exit(0);
            }
        }else{
            //AGUARDA PROCESSO FINALIZAR
            wait(NULL);
        }
    }

}

int main()
{
    char *comandosDeEntrada, *argumentos{MAXLIST};
    char* argumentosPipe[MAXLIST];
    char* nomeArquivo[2];
    char* comandosDeEntradacopy;
    int pipeOuUnitario = 0;
    int direcionadorDuplo;
    char* arquivo_pipe[2];

    while(1)
    {
        printDir();

        nomeArquivo[0] = NULL;
        arquivo_pipe[0] = NULL;

        //LEITURA DA LINHA
        comandosDeEntrada = read_line();

        //SEPARA OS COMANDOS E RETORNA O TIPO DE ABORDAGEM DE EXECUÇÃO
        pipeOuUnitario = processarString(comandosDeEntrada, argumentos, argumentosPipe, nomeArquivo, arquivo_pipe);

        //EXECUTA PIPE OU COMANDOS UNITARIOS
        if (pipeOuUnitario == 1)  executaArgsUnitarios(comandosDeEntradacopy, argumentos, nomeArquivo, 0);

        if (pipeOuUnitario == 2) executaArgsEmPipe(argumentos, argumentosPipe, nomeArquivo, arquivo_pipe, 0);

        if (pipeOuUnitario == 3) executaArgsUnitarios(comandosDeEntradacopy, argumentos, nomeArquivo, 1);

        if (pipeOuUnitario == 4) executaArgsEmPipe(argumentos, argumentosPipe, nomeArquivo, arquivo_pipe, 1);

        //LIBERA A MEMÓRIA ALOCADA
        free(comandosDeEntradaCopy);
        free(comandosDeEntrada);
    }

    //LIBERA A MEMÓRIA ALOCADA PARA OS ARRAYS APÓS O LOOP
    free(argumentos);
    free(argumentosPipe);
    free(nomeArquivo);
    free(arquivo_pipe);

    return 0;
}