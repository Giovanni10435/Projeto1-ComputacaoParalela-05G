#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_LINE 256
#define MAX_SENSOR_ID 20
#define MAX_TYPE 20
#define MAX_STATUS 10

// Estrutura de uma leitura
typedef struct {
    char sensor_id[MAX_SENSOR_ID];
    char data[11];     // YYYY-MM-DD
    char hora[9];      // HH:MM:SS
    char tipo[MAX_TYPE];    // Tipo do Sensor (temperatura, energia, etc.)
    float valor;            // Valor atribuído ao tipo no arquivo, exemplo -> Tipo: Temperatura; Valor: 28°
    char status[MAX_STATUS]; // OK, ALERTA ou CRITICO
} Leitura;

// Função de parsing
int parse_linha(char *linha, Leitura *l) {
    return sscanf(linha, "%s %s %s %s %f status %s",
        l->sensor_id,
        l->data,
        l->hora,
        l->tipo,
        &l->valor,
        l->status);
}

// Leitura do arquivo
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

int main(int argc, char *argv[]) {

    double soma_temp = 0;
    double soma_quad = 0;
    int count_temp = 0;
    int alertas = 0;
    double soma_energia = 0;
    double desvio_padrao = 0;
    
    if (argc < 2) {
        printf("Uso: %s <arquivo.log>\n", argv[0]);
        return 1;
    }

    int total_linhas = 0;

    // Leitura do arquivo
    Leitura *leituras = ler_arquivo(argv[1], &total_linhas);

    printf("Total de linhas lidas: %d\n", total_linhas);

    // Processamento sequencial
    for (int i = 0; i < total_linhas; i++) {
        // Contar alertas
        if(strcmp(leituras[i].status, "ALERTA") == 0 || strcmp(leituras[i].status, "CRITICO") == 0) {
            alertas += 1;
        }
        // Somar energia
        if(strcmp(leituras[i].tipo, "energia") == 0) {
            soma_energia += leituras[i].valor;
        }
        // Calcular média por sensor
        if(strcmp(leituras[i].tipo, "temperatura") == 0) {
            soma_temp += leituras[i].valor;
            count_temp += 1;
        }
    }
    
    double media = soma_temp / count_temp;
    printf("Média das temperaturas por sensor:  %.2f\n", media);

    // Calcular o desvio padrao
    double soma_quadrado = 0;
    for (int i = 0; i < total_linhas; i++) {
        if (strcmp(leituras[i].tipo, "temperatura") == 0) {
            soma_quadrado += pow(leituras[i].valor - media, 2);
        }
    }

    double desvio_padrao = sqrt(soma_quadrado / count_temp);
    
    // Liberação de memória
    free(leituras);
    return 0;
}
