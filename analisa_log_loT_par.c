#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

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

// Variáveis Globais
EstatSensor sensores[MAX_SENSORES];
int total_sensores = 0;
int alertas = 0;
double soma_energia = 0;
pthread_mutex_t lock;

typedef struct {
    char sensor_id[MAX_SENSOR_ID];
    char data[11];
    char hora[9];
    char tipo[MAX_TYPE];
    float valor;
    char status[MAX_STATUS];
} Leitura;

typedef struct {
    Leitura *leituras;
    int inicio;
    int fim;
} ThreadData;

int parse_linha(char *linha, Leitura *l) {
    return sscanf(linha, "%s %s %s %s %f status %s",
        l->sensor_id, 
        l->data, 
        l->hora, 
        l->tipo, 
        &l->valor, 
        l->status);
}

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

    while (fgets(linha, sizeof(linha), fp)) {
        if (count >= capacidade) {
            capacidade *= 2;
            leituras = realloc(leituras, capacidade * sizeof(Leitura));
        }
        if (parse_linha(linha, &leituras[count]) == 6) { count++; }
    }
    fclose(fp);
    *total_linhas = count;
    return leituras;
}

int get_sensor_index(char *id) {
    for (int i = 0; i < total_sensores; i++) {
        if (strcmp(sensores[i].id, id) == 0) return i;
    }
    if (total_sensores < MAX_SENSORES) {
        strcpy(sensores[total_sensores].id, id);
        sensores[total_sensores].soma = 0;
        sensores[total_sensores].soma_quad = 0;
        sensores[total_sensores].count = 0;
        total_sensores++;
        return total_sensores - 1;
    }
    return -1;
}

void* processar_leituras(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    for (int i = data->inicio; i < data->fim; i++) {
        pthread_mutex_lock(&lock);
        if(strcmp(data->leituras[i].status, "ALERTA") == 0 || strcmp(data->leituras[i].status, "CRITICO") == 0) {
            alertas += 1;
        }
        if(strcmp(data->leituras[i].tipo, "energia") == 0) {
            soma_energia += data->leituras[i].valor;
        }
        if(strcmp(data->leituras[i].tipo, "temperatura") == 0) {
            int idx = get_sensor_index(data->leituras[i].sensor_id);
            if (idx != -1) {
                sensores[idx].soma += data->leituras[i].valor;
                sensores[idx].count += 1;
            }
        }
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

void* calcular_variancia(void* arg) {
    ThreadData *data = (ThreadData*)arg;
    for (int i = data->inicio; i < data->fim; i++) {
        if (strcmp(data->leituras[i].tipo, "temperatura") == 0) {
            pthread_mutex_lock(&lock);
            int idx = get_sensor_index(data->leituras[i].sensor_id);
            if (idx != -1) {
                double media_sensor = sensores[idx].soma / sensores[idx].count;
                double diff = data->leituras[i].valor - media_sensor;
                sensores[idx].soma_quad += diff * diff;
            }
            pthread_mutex_unlock(&lock);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    
    if (argc < 3) { 
        printf("Uso: %s <num_threads> <arquivo.log>\n", argv[0]); 
        return 1; 
    }

    int num_threads = atoi(argv[1]);
    char *nome_arquivo = argv[2];

    int total_linhas = 0;
    Leitura *leituras = ler_arquivo(nome_arquivo, &total_linhas);
    printf("Total de linhas lidas: %d\n", total_linhas);

    clock_t inicio_proc = clock();
    pthread_t threads[num_threads];
    ThreadData t_args[num_threads];
    pthread_mutex_init(&lock, NULL);

    int chunk = total_linhas / num_threads;
    for (int i = 0; i < num_threads; i++) {
        t_args[i].leituras = leituras;
        t_args[i].inicio = i * chunk;
        t_args[i].fim = (i == num_threads - 1) ? total_linhas : (i + 1) * chunk;
        pthread_create(&threads[i], NULL, processar_leituras, &t_args[i]);
    }
    for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
    }
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, calcular_variancia, &t_args[i]);
    }
    for (int i = 0; i < num_threads; i++) {
    pthread_join(threads[i], NULL);
    }
    printf("Total de alertas: %d\n", alertas);
    printf("Consumo total de energia: %.2f\n", soma_energia);

    char sensor_instavel[MAX_SENSOR_ID] = "";
    double maior_desvio = 0;
    for (int i = 0; i < total_sensores; i++) {
        if (sensores[i].count > 0) {
            double desvio = sqrt(sensores[i].soma_quad / sensores[i].count);
            if (desvio > maior_desvio) {
                maior_desvio = desvio;
                strcpy(sensor_instavel, sensores[i].id);
            }
        }
    }

    printf("Desvio padrão do sensor mais instável: %.2f\n", maior_desvio);
    printf("Sensor mais instável: %s\n", sensor_instavel);

    printf("\nMédias por sensor (até 10):\n");
    for (int i = 0; i < total_sensores && i < 10; i++) {
        printf("%s -> %.2f\n", sensores[i].id, sensores[i].soma / sensores[i].count);
    }

    pthread_mutex_destroy(&lock);
    free(leituras);
    
    clock_t fim_proc = clock();
    printf("Tempo de execução: %.2f segundos\n", (double)(fim_proc - inicio_proc) / CLOCKS_PER_SEC);

    return 0;
}
