CC = gcc
CFLAGS = -O2
LIBS = -lm -pthread

all: seq par otim

seq: analisa_log_IoT_seq.c
	$(CC) $(CFLAGS) -o seq analisa_log_IoT_seq.c -lm

par: analisa_log_loT_par.c
	$(CC) $(CFLAGS) -o par analisa_log_loT_par.c $(LIBS)

otim: analisa_log_loT_otim.c
	$(CC) $(CFLAGS) -o otim analisa_log_loT_otim.c $(LIBS)

clean:
	rm -f seq par otim
