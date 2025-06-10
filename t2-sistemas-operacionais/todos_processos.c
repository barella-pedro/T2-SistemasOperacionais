#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "processo.h"
#define N 4


void inicializa_tabela(Pag_TP* tabela, int tam);
void inicializa_memoria(Q_Memoria *memoria, int tam);
 
int main(){
    int i, pid, pid_list[N];
    char num_filho[10], pipe_str_w[12];
    //tabelas dos processos filho (Dá para fazer uma função para criar N tabelas, mas por enquanto so 4 para simplificar)

    Pag_TP tabela[4][32];

    //Memoria RAM (Física)
    Q_Memoria mem_principal[16]; inicializa_memoria(mem_principal, 16);

    //Criando o PIPE
    int pipe_fds[4][2];


    for (i = 0; i < N; i++){
        pipe(pipe_fds[i]);
        inicializa_tabela(tabela[i], 32);
        sprintf(pipe_str_w,"%d", pipe_fds[i][1]);
        sprintf(num_filho, "%d",i);
        
        if ((pid = fork()) < -1){
            perror("Erro ao criar processo filho!");
            exit(-1);
        }
        else if (pid == 0){
           sleep(1);            
            if (execl("./processo", "./processo", num_filho, pipe_str_w, NULL ) < 0) {
                perror("Erro ao executar processo filho: ");
                exit(-1);
            }
        }
        else {
            pid_list[i] = pid;
            close(pipe_fds[i][1]); //Aqui fechamos a escrita do pai, pois ele não escreve nada, so recebe instruções
        }
    }
    sleep(4);
    printf("Prestes a entrar\n");

    while (1){
        Instrucao inst_atual;
        for (i = 0; i<N;i++){
            kill(pid_list[i], SIGCONT);
            printf("roundrobin do p%d\n", i);
            read(pipe_fds[i][0], &inst_atual, sizeof(inst_atual));
            printf("Instrucoes: %d, %c\n\n", inst_atual.num_pag, inst_atual.modo);
            sleep(1);
            kill(pid_list[i], SIGSTOP);

            //Para cada processo, vê se a pagina está na memoria ram, se estiver, atualiza as informações necessárias, se não, dá page fault e chama o algoritmo.

            if (!tabela[i][inst_atual.modo].presenca){
                printf("Page Fault!\n");
                for (int j = 0; j<16; j++){
                    if (!mem_principal[j].ativa) {

                        mem_principal[j].ativa = true;
                        mem_principal[j].end_virtual = inst_atual.num_pag;
                        tabela[i][inst_atual.num_pag].moldura = j;
                        tabela[i][inst_atual.num_pag].presenca = true; 
                        printf("Pagina alocada no quadro %d da memoria!\n",j);
                        break;
                    }
                }
                if (!tabela[i][inst_atual.num_pag].presenca) { //se ainda estiver false, a ram esta cheia! Aplicar algum algoritmo
                    printf("Ram Cheia!!!\n");


                }

            } 
            else {
                printf("Pagina ja alocada!\n");
                
            }
        }
    }






    return 0;
}



void inicializa_tabela(Pag_TP* tabela, int tam){
    for (int i = 0; i<tam; i++){
        tabela[i].mod = false;
        tabela[i].presenca = false;
        tabela[i].ref = false;
        tabela[i].moldura = -1;
    }
    return;
}

void inicializa_memoria(Q_Memoria *memoria, int tam){

    for (int i = 0; i < tam; i++){
        memoria[i].ativa = false;
        memoria[i].end_virtual = -1;
        memoria[i].mod = false;
        memoria[i].ref = false;
        memoria[i].pid = -1;
    }
    return;
}