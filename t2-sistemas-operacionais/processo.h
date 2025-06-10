#include <stdbool.h>

typedef struct instrucao{
    int num_pag;
    char modo;
} Instrucao;


struct pag_tabela{
    bool presenca; //se página está na memória RAM
    bool ref; //se houve acesso a algum endereço da página no último intervalo de tempo ∆t
    bool mod; // se houve acesso de escrita a algum endereço da página em ∆t
    int moldura; //quadro na memoria
    //Caching: se a entrada deve ser cacheada?

};//Um array de pag_tabela será a Tabela de Páginas de cada processo
typedef struct pag_tabela Pag_TP;

struct quadro_memoria{
    bool ref;
    bool mod;
    bool ativa; //se esta associada a alguma pagina virtual ou nao
    int pid; //id do processo que esta usando a pagina atualmente
    int end_virtual;//Numero na pagina virtual na tabela do processo X
};
typedef struct quadro_memoria Q_Memoria;