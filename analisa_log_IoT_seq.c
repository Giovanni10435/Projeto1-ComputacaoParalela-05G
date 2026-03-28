#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define MAX_LINE 256
#define MAX_SENSOR_ID 20
#define MAX_TYPE 20
#define MAX_STATUS 10
#define MAX_SENSORES 100

typedef struct {
    char id[MAX_SENSOR_ID];
    double soma;
    double soma_quad;
    int count;
} EstatSensor;

EstatSensor sensores[MAX_SENSORES];
int total_sensores = 0;

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

int get_sensor_index(char *id) {

    for (int i = 0; i < total_sensores; i++) {
        if (strcmp(sensores[i].id, id) == 0)
            return i;
    }

    // novo sensor
    strcpy(sensores[total_sensores].id, id);
    sensores[total_sensores].soma = 0;
    sensores[total_sensores].soma_quad = 0;
    sensores[total_sensores].count = 0;

    total_sensores++;
    return total_sensores - 1;
}

int main(int argc, char *argv[]) {

    clock_t inicio = clock();
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
            int idx = get_sensor_index(leituras[i].sensor_id);

            sensores[idx].soma += leituras[i].valor;
            sensores[idx].count += 1;
}
    }

    printf("Total de alertas: %d\n", alertas);
    printf("Consumo total de energia: %.2f\n", soma_energia);

    // Calcular o desvio padrao
    double soma_quadrado = 0;
    
    char sensor_instavel[MAX_SENSOR_ID] = "";
    double maior_desvio = 0;

    for (int i = 0; i < total_linhas; i++) {
    if (strcmp(leituras[i].tipo, "temperatura") == 0) {
        int idx = get_sensor_index(leituras[i].sensor_id);

        double media_sensor = sensores[idx].soma / sensores[idx].count;
        double diff = leituras[i].valor - media_sensor;

        sensores[idx].soma_quad += diff * diff;
    }
}
    

    desvio_padrao = 0;

    for (int i = 0; i < total_sensores; i++) {
        double media_sensor = sensores[i].soma / sensores[i].count;
        double desvio = sqrt(sensores[i].soma_quad / sensores[i].count);

        if (desvio > maior_desvio) {
            maior_desvio = desvio;
            strcpy(sensor_instavel, sensores[i].id);
        }
    }

    printf("Sensor mais instável: %s (Desvio padrão: %.2f)\n", sensor_instavel, maior_desvio);

    printf("\nMédias por sensor (até 10):\n");

    for (int i = 0; i < total_sensores && i < 10; i++) {
        double media_sensor = sensores[i].soma / sensores[i].count;
        printf("%s -> %.2f\n", sensores[i].id, media_sensor);
    }
    
    // Liberação de memória
    free(leituras);

    clock_t fim = clock();
    double tempo = (double)(fim - inicio) / CLOCKS_PER_SEC;
    printf("Tempo de execução: %.2f segundos\n", tempo);

    return 0;
}
