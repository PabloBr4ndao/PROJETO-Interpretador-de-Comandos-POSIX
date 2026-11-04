#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 100
#define MAX_LINE 1024

// Função para tratar processos externos e redirecionamentos
void executar_comando(char **args) {
    int i = 0;
    int redirecionar_in = 0, redirecionar_out = 0, append_out = 0;
    char *arquivo_in = NULL;
    char *arquivo_out = NULL;

    // Varredura para identificar operadores de redirecionamento
    while (args[i] != NULL) {
        if (strcmp(args[i], "<") == 0) {
            redirecionar_in = 1;
            arquivo_in = args[i + 1];
            args[i] = NULL; // Interrompe os argumentos para o execvp não ler o "<"
        } else if (strcmp(args[i], ">") == 0) {
            redirecionar_out = 1;
            arquivo_out = args[i + 1];
            args[i] = NULL; 
        } else if (strcmp(args[i], ">>") == 0) {
            append_out = 1;
            arquivo_out = args[i + 1];
            args[i] = NULL;
        }
        if (args[i] != NULL) i++;
    }

    // Criação do processo filho
    pid_t pid = fork();

    if (pid == -1) {
        perror("Erro ao executar fork");
    } else if (pid == 0) {
        // --- BLOCO DO PROCESSO FILHO ---
        
        // Trata redirecionamento de entrada (<)
        if (redirecionar_in) {
            int in_fd = open(arquivo_in, O_RDONLY);
            if (in_fd < 0) { perror("Erro ao abrir arquivo de entrada"); exit(1); }
            dup2(in_fd, STDIN_FILENO); // Substitui stdin pelo arquivo
            close(in_fd);
        }
        
        // Trata redirecionamento de saída sobrescrevendo (>)
        if (redirecionar_out) {
            int out_fd = open(arquivo_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) { perror("Erro ao abrir arquivo de saída"); exit(1); }
            dup2(out_fd, STDOUT_FILENO); // Substitui stdout pelo arquivo
            close(out_fd);
        } 
        // Trata redirecionamento de saída concatenando (>>)
        else if (append_out) {
            int out_fd = open(arquivo_out, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0) { perror("Erro ao abrir arquivo de saída"); exit(1); }
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        // Executa o binário do comando
        if (execvp(args[0], args) == -1) {
            perror("Comando não encontrado");
            exit(1);
        }
    } else {
        // --- BLOCO DO PROCESSO PAI ---
        // Aguarda a conclusão do processo filho
        waitpid(pid, NULL, 0);
    }
}

int main() {
    char linha[MAX_LINE];
    char *args[MAX_ARGS];
    
    while (1) {
        printf("myshell> ");
        fflush(stdout);

        // Lê a entrada do usuário
        if (fgets(linha, MAX_LINE, stdin) == NULL) break;

        // Remove o caractere de nova linha '\n'
        linha[strcspn(linha, "\n")] = 0; 

        if (strlen(linha) == 0) continue;

        // Tokenização (divisão da string por espaços)
        int i = 0;
        args[i] = strtok(linha, " ");
        while (args[i] != NULL && i < MAX_ARGS - 1) {
            i++;
            args[i] = strtok(NULL, " ");
        }

        if (args[0] == NULL) continue;

        // Comando embutido: exit
        if (strcmp(args[0], "exit") == 0) {
            break;
        }

        // Comando embutido: cd (Mudar de diretório)
        if (strcmp(args[0], "cd") == 0) {
            if (args[1] == NULL) {
                fprintf(stderr, "cd: argumento esperado (destino)\n");
            } else {
                if (chdir(args[1]) != 0) {
                    perror("cd falhou");
                }
            }
            continue; // Pula o fork, pois 'cd' é executado no próprio shell
        }

        // Se não for um comando embutido, executa via fork/exec
        executar_comando(args);
    }
    return 0;
}
