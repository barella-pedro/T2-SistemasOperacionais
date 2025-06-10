#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include "processo.h"


void testa_arq(FILE* arquivo);
void cont_handler(int sinal){
    printf("Fui continuado!\n");
    return;
}


int main(int argc, char* argv[]){
    if (argc < 2){
        printf("Uso correto: %s <numero_do_processo> <pipe_fd_w>\n",argv[0]);
        exit(1);
    }
    signal(SIGCONT, cont_handler);
    FILE *arq_instrucoes;
    Instrucao lista_instrucoes[100];
    int num_processo = atoi(argv[1]) + 1;
    int pipe_fd = atoi(argv[2]);
    printf("%d\n",pipe_fd);
    int i = 0;
    char nome_arq[40], buffer_linha[10];

    //Alterando o nome do arquivo para o processo X
    snprintf(nome_arq, 40, "acessos/acessos_p%d.txt", num_processo);
    printf("Processo: %d\n",num_processo);
    printf("Arquivo: %s\n",nome_arq);
    arq_instrucoes = fopen(nome_arq, "r");
    testa_arq(arq_instrucoes);

    //Leitura do arquivo para instruções
    while(fgets(buffer_linha, sizeof(buffer_linha), arq_instrucoes)){
        if (sscanf(buffer_linha, "%d %c", &lista_instrucoes[i].num_pag, &lista_instrucoes[i].modo) == 2){
            // printf("%d, %c - %d\n", lista_instrucoes[i].num_pag, lista_instrucoes[i].modo, i);
            i++;
        }
        else printf("Linha no formato errado.\n");
    }
    printf("Pausei\n");
    pause(); //espera pelo RoundRobin

    for (i = 0; i <100; i++){//depois criar variavel para substuir o "100"
        write(pipe_fd, &lista_instrucoes[i], sizeof(lista_instrucoes[i]));
        printf("Enviei pro pai a %dº linha!\n", i+1);
        pause();//pause para evitar que ele envie mais de uma instrução
        printf("Depois do Pause\n");

    }
    close(pipe_fd); 
    fclose(arq_instrucoes);
    return 0;
}


void testa_arq(FILE* arquivo){
    if (!arquivo){
        perror("Erro ao abrir o arquivo!");
        exit(-1);
    }
    else return;
}

