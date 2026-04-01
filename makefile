CC = gcc
CFLAGS = -O2
LIBS = -lm -pthread

all: seq par otim

seq: sensor_analyzer_seq.c
	$(CC) $(CFLAGS) -o seq sensor_analyzer_seq.c -lm

par: sensor_analyzer_par.c
	$(CC) $(CFLAGS) -o par sensor_analyzer_par.c $(LIBS)

otim: sensor_analyzer_optimized.c
	$(CC) $(CFLAGS) -o otim sensor_analyzer_optimized.c $(LIBS)

clean:
	rm -f seq par otim
