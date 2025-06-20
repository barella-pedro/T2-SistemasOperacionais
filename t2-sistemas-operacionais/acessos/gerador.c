#include <stdio.h>
#include <stdlib.h>
#include <time.h> // Para gerar números aleatórios

int main(int argc, char *argv[]) {
    // Verifica se os parâmetros foram passados corretamente
    if (argc != 3) {
        printf("Uso: %s <Nlinhas> <nome_arq>\n", argv[0]);
        printf("Exemplo: %s 500 meu_arquivo.txt\n", argv[0]);
        return 1; // Retorna erro
    }

    // Converte o número de linhas de string para inteiro
    int Nlinhas = atoi(argv[1]);
    if (Nlinhas <= 0) {
        printf("Nlinhas deve ser um número inteiro positivo.\n");
        return 1;
    }

    // Obtém o nome do arquivo
    char *nome_arq = argv[2];

    // Abre o arquivo para escrita
    FILE *arquivo = fopen(nome_arq, "w");
    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo %s\n", nome_arq);
        return 1;
    }

    // Inicializa o gerador de números aleatórios com o tempo atual
    srand((unsigned int)time(NULL));

    // Preenche o arquivo com Nlinhas
    for (int i = 0; i < Nlinhas; i++) {
        // Gera um número aleatório entre 0 e 31 (inclusive)
        int numero = rand() % 32;

        // Gera uma operação aleatória ('R' ou 'W')
        char operacao = (rand() % 2 == 0) ? 'R' : 'W';

        // Escreve a linha no arquivo no formato "XX O"
        fprintf(arquivo, "%02d %c\n", numero, operacao);
    }

    // Fecha o arquivo
    fclose(arquivo);

    printf("Arquivo '%s' com %d linhas gerado com sucesso!\n", nome_arq, Nlinhas);

    return 0; // Retorna sucesso
}