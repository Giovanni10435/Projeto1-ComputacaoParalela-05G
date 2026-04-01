# Variáveis de Compilação
CC = gcc
CFLAGS = -Wall -O3
LIBS = -lpthread -lm

# Nomes dos Executáveis
EXE_SEQ = sensor_analyzer_seq
EXE_PAR = sensor_analyzer_par
EXE_OPT = sensor_analyzer_optimized

# Arquivos Fonte (Ajuste os nomes se forem diferentes no seu PC)
SRC_SEQ = sensor_analyzer_seq.c
SRC_PAR = sensor_analyzer_par.c
SRC_OPT = sensor_analyzer_optimized.c

# Regra principal: compila tudo
all: $(EXE_SEQ) $(EXE_PAR) $(EXE_OPT)

# Compilação da versão Sequencial
$(EXE_SEQ): $(SRC_SEQ)
	$(CC) $(CFLAGS) $(SRC_SEQ) -o $(EXE_SEQ) $(LIBS)

# Compilação da versão Paralela (Mutex)
$(EXE_PAR): $(SRC_PAR)
	$(CC) $(CFLAGS) $(SRC_PAR) -o $(EXE_PAR) $(LIBS)

# Compilação da versão Otimizada
$(EXE_OPT): $(SRC_OPT)
	$(CC) $(CFLAGS) $(SRC_OPT) -o $(EXE_OPT) $(LIBS)

# Limpeza dos arquivos binários
clean:
	rm -f $(EXE_SEQ) $(EXE_PAR) $(EXE_OPT)
	@echo "Arquivos temporários removidos com sucesso!"

# Ajuda
help:
	@echo "Comandos disponíveis:"
	@echo "  make       - Compila as 3 versões do projeto"
	@echo "  make clean - Remove os executáveis gerados"
