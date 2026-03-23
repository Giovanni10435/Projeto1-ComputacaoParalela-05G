#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 256
#define MAX_SENSOR_ID 20
#define MAX_TYPE 20
#define MAX_STATUS 10

// =======================
// Estrutura de uma leitura
// =======================
typedef struct {
    char sensor_id[MAX_SENSOR_ID];
    char data[11];     // YYYY-MM-DD
    char hora[9];      // HH:MM:SS
    char tipo[MAX_TYPE];
    float valor;
    char status[MAX_STATUS];
} Leitura;

// =======================
// Função de parsing
// =======================
int parse_linha(char *linha, Leitura *l) {
    return sscanf(linha, "%s %s %s %s %f status %s",
                  l->sensor_id,
                  l->data,
                  l->hora,
                  l->tipo,
                  &l->valor,
                  l->status);
}

// =======================
// Leitura do arquivo
// =======================
Leitura* ler_arquivo(const char *nome_arquivo, int *total_linhas) {
    FILE *fp = fopen(nome_arquivo, "r");
    if (!fp) {
        perror("Erro ao abrir arquivo");
        exit(1);
    }

    char linha[MAX_LINE];
    int capacidade = 1000;
    int count = 0;

    Leitura *leituras = malloc(capacidade * sizeof(Leitura));
    if (!leituras) {
        perror("Erro de memória");
        exit(1);
    }

    while (fgets(linha, sizeof(linha), fp)) {
        if (count >= capacidade) {
            capacidade *= 2;
            leituras = realloc(leituras, capacidade * sizeof(Leitura));
            if (!leituras) {
                perror("Erro de realloc");
                exit(1);
            }
        }

        if (parse_linha(linha, &leituras[count]) == 6) {
            count++;
        }
    }

    fclose(fp);
    *total_linhas = count;
    return leituras;
}

// =======================
// MAIN
// =======================
int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Uso: %s <arquivo.log>\n", argv[0]);
        return 1;
    }

    int total_linhas = 0;

    // Leitura do arquivo
    Leitura *leituras = ler_arquivo(argv[1], &total_linhas);

    printf("Total de linhas lidas: %d\n", total_linhas);

    // =======================
    // Processamento sequencial
    // =======================
    for (int i = 0; i < total_linhas; i++) {
        // TODO:
        // Aqui você vai:
        // - contar alertas
        // - somar energia
        // - calcular média por sensor
        // - preparar desvio padrão
    }

    // =======================
    // Liberação de memória
    // =======================
    free(leituras);

    return 0;
}
