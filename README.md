# Shell em C
Projeto que tem como objetivo fazer um interpretador de comandos.
<br>
O programa executa os seguintes comandos:
<br>
-path
<br>
-cd
<br>
-clear
<br>
-ls
<br>
-cat
<br>
-echo
<br>
-exit

## Como executar
1. ### Compilação
Compile o programa usando o compilador C compatível, como o GCC
```sh
gcc -c shell.c
```
```sh
gcc -o shell shell.o
```
2. ### Execução
Após compilar , execute o programa da forma desejada
<br>
a) Execute o shell
```sh
./shell
```
b) Execute um arquivo em uma lista de comandos
```sh
./shell <nome_do_arquivo>
```
Para executar os comandos da pasta Comandos.txt basta modificar o path add
```sh
path add <caminho1> <caminho2> ...
```
## Comandos
Ao executar o programa você deve colocar os caminhos necessarios para que o Shell execute os comandos corretamente.
<br>
Para isso faça o seguinte comando:
<br>
a) Para adicionar algum caminho no path
```sh
path add <caminho1> <caminho2> ...
```
b) Para remover algum caminho do path
```Sh
path remove <caminho1> <caminho2> ...
```
c) Para ver os caminhos que estão no path
```sh
path
```
### Limpar a tela
```sh
clear
```
### Entrar em um diretório
```sh
cd <nome_do_diretório>
```
### Para listagem de diretórios e arquivos
a) Diretórios e arquivos não ocultos
```sh
ls
```
b) Todos os diretórios e arquivos
```sh
ls -a
```
c) Informações dos arquivos e diretórios não ocultos
```sh
ls -l
```
d) Informações de todos os arquivos e diretórios
```sh
ls -al
```
```sh
ls -la
```
```sh
ls -a -l
```
```sh
ls -l -a
```
### Manipulação de conteúdo de arquivos
a) Impressão na tela
```sh
cat <nome_do_arquivo>
```
b) Copiar o conteúdo de um arquivo em outro
```sh
cat <nome_do_arquivo_original> > <nome_do_arquivo_destino>
```
### Imprimir mensagem na tela
```sh
echo <mensagem>
````
### Fechar o Shell
```sh
exit
```
