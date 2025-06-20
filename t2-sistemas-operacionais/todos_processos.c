#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "processo.h"
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define N 4
#define NUM_FRAMES 16
#define NUM_PAGES 32

void inicializa_tabela(Pag_TP* tabela, int tam);
void inicializa_memoria(Q_Memoria *memoria, int tam);
void print_memoria(Q_Memoria memoria[], int tamanho);
int select_NRU(Q_Memoria memoria[], int tamanho);
int select_sec_chance(Q_Memoria memoria[], int vet_sec_chance[], int tam);

//* Pointer da Second Chance
int pointer;



int main(int argc, char* argv[]){
    if (argc < 1){
        printf("Uso correto: %s <algoritmo>\n",argv[0]);
        exit(1);
    }

    int i, pid, pid_list[N];
    char num_filho[10], pipe_str_w[12];
    char *algoritmo = argv[1];
    printf("%s\n", algoritmo);
    /* tabelas de páginas dos 4 processos */
    Pag_TP tabela[N][NUM_PAGES];

    /* memória física */
    Q_Memoria mem_principal[NUM_FRAMES];
    inicializa_memoria(mem_principal, NUM_FRAMES);

    /* pipes para cada filho */
    int pipe_fds[N][2];

    /* criar processos filhos */
    for (i = 0; i < N; i++) {
        pipe(pipe_fds[i]);
        inicializa_tabela(tabela[i], NUM_PAGES);

        sprintf(pipe_str_w, "%d", pipe_fds[i][1]);
        sprintf(num_filho, "%d", i);

        if ((pid = fork()) < 0) {
            perror("Erro ao criar processo filho");
            exit(1);
        }
        else if (pid == 0) {
            /* child */
            sleep(1);
            execl("./processo", "./processo", num_filho, pipe_str_w, NULL);
            perror("Erro ao executar processo filho");
            exit(1);
        }
        else {
            /* parent */
            pid_list[i] = pid;
            close(pipe_fds[i][1]);
        }
    }

    sleep(4);
    printf("Prestes a entrar\n");

    // Medição de tempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    bool ativos[N] = {true, true, true, true};
    int processos_ativos = 4;

    int iteracao = 0;
    int p_faults = 0;
    int rounds = 0;
    srand(time(NULL));

    
    //* Ferramentas utilizadas por algoritmo:
    int bits_sec_chance[NUM_FRAMES];
    
    if (strcmp(algoritmo, "SECONDCHANCE") == 0){
        memset(bits_sec_chance, 0, sizeof(bits_sec_chance));
        pointer = 0;
    }
    
    
    
    while (processos_ativos>0) {
        Instrucao inst_atual;
        
        for (i = 0; i < N; i++) {
            /* Round robin: libera e pausa o filho */
            if (strcmp(algoritmo, "SECONDCHANCE") == 0) {
                printf("Vetor de bits:\n");
                for (int k = 0; k<NUM_FRAMES; k++){
                    printf("%d ",bits_sec_chance[k]);
                }
                printf("\n");
                printf("Pointer: %d\n", pointer);
            }
            
            if (!ativos[i]) continue;
            
            kill(pid_list[i], SIGCONT);
            printf("roundrobin do p%d\n", i);
            size_t lido = read(pipe_fds[i][0], &inst_atual, sizeof(Instrucao));
            
            if (lido == 0){
                printf("Processo parou de mandar instruções!!\n");
                ativos[i] = false;
                processos_ativos--;
                continue;
            }
            
            
            printf("Instrucoes: %d, %c\n\n", inst_atual.num_pag, inst_atual.modo);
            sleep(0.5);
            // kill(pid_list[i], SIGSTOP);

            /* Page fault? */
            if (!tabela[i][inst_atual.num_pag].presenca) {
                printf("Page Fault!\n");
                p_faults++;
                /* tenta quadro livre */
                bool alocou = false;
                for (int j = 0; j < NUM_FRAMES; j++) {
                    if (!mem_principal[j].ativa) {
                        /* aloca aqui */
                        mem_principal[j].ativa        = true;
                        mem_principal[j].end_virtual  = inst_atual.num_pag;
                        mem_principal[j].pid          = pid_list[i];
                        mem_principal[j].conteudo     = i + 1;

                        /* marca bits R/M */
                        mem_principal[j].ref          = true;
                        mem_principal[j].mod          = (inst_atual.modo == 'W'); 

                        tabela[i][inst_atual.num_pag].moldura  = j;
                        tabela[i][inst_atual.num_pag].presenca = true;
                        tabela[i][inst_atual.num_pag].ref      = true;
                        tabela[i][inst_atual.num_pag].mod      = (inst_atual.modo == 'W');
                        
                        printf("Pagina alocada no quadro %d da memoria!\n", j);
                        alocou = true;
                        break;
                    }
                }

                /* se não alocou, RAM cheia → aplica Algoritmo */ 
                if (!alocou) {
                    printf("Ram Cheia!!!\n");
                    int vitima;
                    
                    //* NRU
                    if (strcmp(algoritmo, "NRU") == 0){ 
                    /* seleciona vítima */
                        vitima = select_NRU(mem_principal, NUM_FRAMES);
                        if (vitima < 0) {
                            fprintf(stderr, "NRU não encontrou vítima\n");
                            exit(1);
                        }
                    }
                    
                    
                    //* Second Chance
                    else if (strcmp(algoritmo, "SECONDCHANCE") == 0){
                        vitima = select_sec_chance(mem_principal, bits_sec_chance ,NUM_FRAMES);
                        printf("Vitima escolhida: %d\n",vitima);
                        printf("Pointer esta em %d\n", pointer);


                    }
                    
                    
                    //* Não encontrou a instrução, sai do programa
                    else {
                        printf("Instrução \"%s\" não encontrada!\n", algoritmo);
                        exit(-2);
                    }
                    //*
                    

                    /* identifica processo vítima */
                    int pid_vit_idx = -1;
                    for (int p = 0; p < N; p++) {
                        if (pid_list[p] == mem_principal[vitima].pid) {
                            pid_vit_idx = p;
                            break;
                        }
                    }

                    int pag_vit = mem_principal[vitima].end_virtual;
                    if (mem_principal[vitima].mod) {
                        printf("Página suja! Gravando em swap...\n");
                    }

                    /* limpa tabela da vítima */
                    tabela[pid_vit_idx][pag_vit].presenca = false;
                    tabela[pid_vit_idx][pag_vit].moldura  = -1;
                    
                    /* aloca nova página no mesmo quadro */
                    mem_principal[vitima].end_virtual = inst_atual.num_pag;
                    mem_principal[vitima].pid         = pid_list[i];
                    mem_principal[vitima].conteudo    = i + 1;
                    mem_principal[vitima].ref         = true;
                    mem_principal[vitima].mod         = (inst_atual.modo == 'W');
                    
                    /* atualiza tabela do processo atual */
                    tabela[i][inst_atual.num_pag].presenca = true;
                    tabela[i][inst_atual.num_pag].moldura  = vitima;
                    tabela[i][inst_atual.num_pag].ref      = true;
                    tabela[i][inst_atual.num_pag].mod      = (inst_atual.modo == 'W');

                    printf("Substituiu quadro %d pela página %d de P%d\n",
                           vitima, inst_atual.num_pag, i);
                }
            }
            else {
                /* página já na RAM: atualiza bits */
                printf("Pagina ja alocada!\n");
                int moldura = tabela[i][inst_atual.num_pag].moldura;

                /* marca referência */
                tabela[i][inst_atual.num_pag].ref = true;
                mem_principal[moldura].ref       = true;
                
                /* marca modificação só em write */
                if (inst_atual.modo == 'W') {
                    tabela[i][inst_atual.num_pag].mod = true;
                    mem_principal[moldura].mod       = true;
                }

                //* Second Chance
                if (strcmp(algoritmo, "SECONDCHANCE") == 0){
                    bits_sec_chance[moldura] = 1;
                }
            }
        rounds++;
        
        }
        
        /* incrementa iteração e zera R a cada 10 ciclos */
        
        //* A principio apenas NRU está utilizando isso (por enquanto)
        iteracao++;
        if (iteracao > 0 && iteracao % 10 == 0) {
            printf("Zerando os bits de referência (NRU)....\n");
            for (int k = 0; k < NUM_FRAMES; k++) {
                mem_principal[k].ref = false;
            }
        }
        
        /* mostra estado da RAM */
        print_memoria(mem_principal, NUM_FRAMES);
    }
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = (end_time.tv_sec - start_time.tv_sec) + (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    printf("Rounds: %d\n", rounds);
    printf("Page Faults: %d\n", p_faults);
    printf("Tempo de execução: %.6f segundos\n", elapsed);

    return 0;
}







//* Funções auxiliares

void inicializa_tabela(Pag_TP* tabela, int tam) {
    for (int i = 0; i < tam; i++) {
        tabela[i].mod      = false;
        tabela[i].presenca = false;
        tabela[i].ref      = false;
        tabela[i].moldura  = -1;
    }
}

void inicializa_memoria(Q_Memoria *memoria, int tam) {
    for (int i = 0; i < tam; i++) {
        memoria[i].ativa       = false;
        memoria[i].end_virtual = -1;
        memoria[i].mod         = false;
        memoria[i].ref         = false;
        memoria[i].pid         = -1;
        memoria[i].conteudo    = 0;
    }
}

void print_memoria(Q_Memoria memoria[], int tamanho) {
    printf("\n=== ESTADO DA MEMÓRIA FÍSICA ===\n");
    for (int i = 0; i < tamanho; i++) {
        if (memoria[i].ativa) {
            printf("Quadro %2d | PID: %d | Pág: %2d | R: %d | M: %d | Conteúdo: %d\n",
                   i,
                   memoria[i].pid,
                   memoria[i].end_virtual,
                   memoria[i].ref,
                   memoria[i].mod,
                   memoria[i].conteudo);
        } else {
            printf("Quadro %2d | VAZIO\n", i);
        }
    }
    printf("=================================\n\n");
}

/* Função NRU: escolhe aleatoriamente uma moldura da menor classe */
int select_NRU(Q_Memoria memoria[], int tamanho) {
    int candidatos[4][NUM_FRAMES];
    int cont[4] = {0};

    /* agrupa por classe (2*R + M) */
    for (int i = 0; i < tamanho; i++) {
        if (!memoria[i].ativa) continue;
        int classe = 2 * memoria[i].ref + memoria[i].mod; // Classe 0)  0 + 0 = 0 ; Classe 1) 0 + 1 = 1; Classe 2) 2 + 0 = 2; Classe 3) 2 + 1 = 3; Sendo a 3 a menos propensa a sair
        candidatos[classe][cont[classe]++] = i;
    }

    /* escolhe primeira classe não vazia */
    for (int c = 0; c < 4; c++) {
        if (cont[c] > 0) {
            if ((c == 1) || (c == 4)) {
                printf("Um quadro modificado foi removido! Realizando SWAP para o disco!"); //Caso o quadro tenha sido alterado, deve-se salvar na memora a alteração
            }
            int idx = rand() % cont[c];
            return candidatos[c][idx];
        }
    }



    return -1;
}


int select_sec_chance(Q_Memoria memoria[], int vet_sec_chance[], int tam){
    
    //Verificando se o quadro referenciado pelo pointer atual poder ser substituido:
    int vitima = -1;
    while (vitima < 0){
        if (vet_sec_chance[pointer] == 0){
            
            vitima = pointer;
            pointer = (pointer + 1) % tam;
            return vitima;
        }
        else if (vet_sec_chance[pointer] == 1){
            vet_sec_chance[pointer] = 0;
            pointer = (pointer + 1) % tam;
        }
    }

}
