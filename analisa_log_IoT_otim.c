#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_LINHA 256
#define MAX_SENSOR_ID 20
#define MAX_TYPE 20
#define MAX_STATUS 10
#define MAX_SENSORES 100

/*---------- Estruturas de Dados ----------*/

typedef struct {
	char id[MAX_SENSOR_ID];
	double soma;
	double soma_quad;
	int count;
} EstatSensor;

typedef struct {
	char sensor_id[MAX_SENSOR_ID];
	char data[11];
	char hora[9];
	char tipo[MAX_TYPE];
	float valor;
	char status[MAX_STATUS];
} Leitura;
typedef struct {
	int alertas;
	double soma_energia;
	EstatSensor sensores[MAX_SENSORES];
	int total_sensores;
} ResultadosFase1;

typedef struct {
	double soma_quad[MAX_SENSORES];	
} ResultadoFase2;

typedef struct {
	Leitura *leituras;
	int inicio;
	int fim;
	EstatSensor *sensores_globais;
	int total_sensores_globais;
} ThreadData;

/* ---------- Variaveis Globais ---------- */

EstatSensor sensores[MAX_SENSORES];
int total_sensores = 0;

/* ---------- Line Parsing ---------- */

int parse_linha(char *linha, Leitura *l) {
	return sscanf(linha, "%19s %10s %8s %19s %f status %9s",
				l->sensor_id, l->data, l->hora,
				l->tipo, &l->valor, l->status);
}

/* ---------- Leitura do Arquivo ---------- */

Leitura *ler_arquivo(const char *nome_arquivo, int *total_linhas) {
	FILE *fp = fopen(nome_arquivo, "r");
	if (!fp) { perror("Error ao abrir arquivo"); exit(1); }
	
	char linha[MAX_LINHA];
	int capacidade = 1000, count = 0;
	Leitura *leituras = malloc(capacidade * sizeof(Leitura));
	if (!leituras) { perror("malloc"); exit(1); }
	
	while (fgets(linha, sizeof(linha), fp)) {
		if (count >= capacidade) {
			capacidade *= 2;
			leituras = realloc(leituras, capacidade * sizeof(Leitura));
			if (!leituras) { perror("realloc"); exit(1); }
		}
		if (parse_linha(linha, &leituras[count]) == 6) count++;
	}
	fclose(fp);
	*total_linhas = count;
	return leituras;
}

/* ---------- Busca Sensor ---------- */

static int get_local_sensor_index(EstatSensor *tab, int *total, const char *id) {
	for (int i = 0; i < *total; i++) {
		if (strcmp(tab[i].id, id) == 0) return i;
	}
	
	if (*total < MAX_SENSORES) {
		int idx = (*total)++;
		strncpy(tab[idx].id, id, MAX_SENSOR_ID - 1);
		tab[idx].id[MAX_SENSOR_ID - 1] = '\0';
		tab[idx].soma = 0.0;
		tab[idx].soma_quad = 0.0;
		tab[idx].count = 0;
		return idx;
	}
	return -1;
}

void *processar_leituras(void *arg) {
	ThreadData *data = (ThreadData *)arg;
	ResultadoFase1 *res = malloc(sizeof(Resultadofase1));
	if (!res) { perror("malloc"); pthread_exit(NULL); }
	
	res->alertas = 0;
	res->soma_energia = 0.0;
	res->total_sensores = 0;
	memset(res->sensores, 0, sizeof(res->sensores));
	
	for (int i = data->inicio; i < data->fim; i++) {
		Leitura *l = &data->leituras[i];
		
		if (strcmp(l->status, "ALERTA") == 0 ||
			strcmp(l->status, "CRITICO") == 0) {
				res->alertas++;
			}
		
		if (strcmp(l->tipo, "energia") == 0) {
			res->soma_energia += l->valor;
		}
		
		if (strcmp(l->tipo, "temperatura") == 0) {
			int idx = get_local_sensor_index(res->sensores,
											&res->total_sensores,
											l->sensor_id);
			if (idx != -1) {
				res-sensores[idx].soma += l->valor;
				res->sensores[idx].count += 1;
			}
		}
	}
	return (void *)res;
}

static void reduzir_fase1(ResultadoFase1 **partes, int n,
						int *alertas_out, double *energia_out) {
	int alertas = 0;
	double soma_energia = 0.0;
	
	for (int t = 0; t < n; t++) {
		alertas += partes[t]->alertas;
		soma_energia += partes[t]->soma_energia;
		
		for (int s = 0; s < partes[t]->total_sensores; s++) {
			EstatSensor *loc = &partes[t]->sensores[s];
			
			int idx = -1;
			for (int g = 0; g < total_sensores; g++) {
				if (strcmp(sensores[g].id, loc->id) == 0) { idx = g; break }
			}
			if (idx == -1 && total_sensores < MAX_SENSORES) {
				idx = total_sensores++;
				strcnpy(sensores[idx].id, loc->id, MAX_SENSOR_ID - 1);
				sensores[idx].id[MAX_SENSOR_ID -1] = '\0';
				sensores[idx].soma = 0.0;
				sensores[idx].soma_quad = 0.0;
				sensores[idx].count = 0;
			}
			if (idx != -1) {
				sensores[idx].soma += loc->soma;
				sensores[idx].count += loc->count;
			}
		}
	}
	*alertas_out = alertas;
	*energia_out = soma_energia;
}

void *calcular_variancia(void *arg) {
	ThreadData *data = (ThreadData *)arg;
	ResultadoFase2 *res = malloc(sizeof(ResultadoFase2));
	if (!res) { perror("malloc"); pthread_exit(NULL); }
	
	memset(res->soma_quad, 0, sizeof(res->soma_quad));
	
	for (int i = data->inicio; i < data->fim; i++) {
		Leitura *l = &data->leituras[i];
		if (strcmp(l->tipo, "temperatura") != 0) continue;
		
		for (int g = 0; g < data->total_sensores_globais; g++) {
			if (strcmp(data->sensores_globais[g].id, l->sensor_id) == 0) {
				double media = data->sensores_globais[g].soma /
								data->sensores_globais[g].count;
				double diff = l->valor - media;
				res->soma_quad[g] += diff * diff;
				break;
				
			}
		}
	}
	return (void *)res;
}

static void reduzir_fase2(ResultadoFase2 **partes, int n) {
	for (int t = 0; t < n; t++) {
		for (int g = 0; g < total_sensores; g++) {
			sensores[g].soma_quad += partes[t]->soma_quad[g];
		}
	}
}

/* ---------- Main ---------- */

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Uso: %s <num_threads> <arquivo.log>\n", argv[0]);
		return 1;
	}
	
	int num_threads = atoi(argv[1]);
	char *nome_arquivo = argv[2];
	
	if (num_threads <= 0) {
		fprintf(stderr, "Numero de threads deve ser >= 1\n");
		return 1;
	}
	
	int total_linhas = 0;
	Leitura *leituras = ler_arquivo(nome_arquivo, &total_linhas);
	printf("Total de linhas lidas: %d\n", total_linhas);
	
	struct timespec ts_inicio, ts_fim;
	clock_gettime(CLOCK_MONOTONIC, &ts_inicio);
	
	pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
	ThreadData *t_args = malloc(num_threads * sizeof(ThreadData));
	if (!threads || !t_args) { perror("malloc"); return 1; }
	
	int chunk = total_linhas / num_threads;
	for (int i = 0; i < num_threads; i++) {
		t_args[i].leituras = leituras;
		t_args[i].inicio = i * chunk;
		t_args[i].fim = (i == num_threads - 1) ? total_linhas : (i + 1) * chunk;
		t_args[i].sensores_globais = sensores;
		t_args[i].total_sensores_globais = 0;
	}
	
	for (int i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, processar_leituras, &t_args[i]);
	}
	
	ResultadoFase1 **res1 = malloc(num_threads * sizeof(ResultadoFase1 *));
	if (!res1) { perror("malloc"); return 1; }
	
	for (int i = 0; i < num_threads; i++) {
		pthread_join(threads[i], (void **)&res1[i]);
	}
	
	int alertas = 0;
	double soma_energia = 0.0;
	reduzir_fase1(res1, num_threads, &alertas, &soma_energia);
	for (int i = 0; i < num_threads; i++) free(res1[i]);
	free(res1);
	
	for (int i = 0; i < num_threads; i++) {
		t_args[i].total_sensores_globais = total_sensores;
	}
	
	for (int i = 0; i < num_threads; i++) {
		pthread_create(&threads[i], NULL, calcular_variancia, &t_args[i]);
	}
	
	ResultadoFase2 **res2 = malloc(num_threads * sizeof(ResultadoFase2 *));
	if (!res2) { perror("malloc"); return 1; }
	
	for (int i = 0; i < num_threads; i++) {
		pthread_join(threads[i], (void **)&res2[i]);
	}
	
	reduzir_fase2(res2, num_threads);
	for (int i = 0; i < num_threads; i++) free(res2[i]);
	free(res2);
	
	printf("Total de alertas: %d\n", alertas);
	printf("Consumo total de energia: %.2f\n", soma_energia);
	
	char sensor_instavel[MAX_SENSOR_ID] = "";
	double maior_desvio = 0.0;
	
	for (int i = 0; i < total_sensores; i++) {
		if (sensores[i].count > 1) {
			double desvio = sqrt(sensores[i].soma_quad / (sensores[i].count - 1));
			if (desvio > maior_desvio) {
				maior_desvio = desvio;
				strcnpy(sensor_instavel, sensores[i].id, MAX_SENSOR_ID - 1);
				sensor_instavel[MAX_SENSORE_ID - 1] = '\0';
			}
		}
	}
	
	printf("Sensor mais instavel: %s\n", sensor_instavel);
	printf("Desvio padrao do sensor mais instavel: %.2f\n", maior_desvio);
	
	printf("\nMedias por sensor (ate o 10):\n");
	for (int i = 0; i < total_sensores && i < 10; i++) {
		if (sensores[i].count > 0) {
			printf("	%s -> %.2f\n",
					sensores[i].id,
					sensores[i].soma / sensores[i].count);
		}
	}
	free(leituras);
	free(threads);
	free(t_args);
	
	clock_gettime(CLOCK_MONOTONIC, &ts_fim);
	double elapsed = (ts_fim.tv_sec - ts_inicio.tv_sec) +
					(ts_fim.tv_sec - ts_inicio.tv_nsec) / 1e9;
	printf("\nTempo de execucao: %.4f segunds\n", elapsed);
	
	return 0;
}
