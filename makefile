CC = gcc
CFLAGS = -Wall -Wextra -02 -pthread
LDFLAGS = -lm -pthread

TARGETS = sensor_analyzer_seq sensor_analyzer_par sensor_analyzer_optimized

all: $(TARGETS)

sensor_analyzer_seq: analisa_log_IoT_seq.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sensor_analyzer_par: analisa_log_IoT_par.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

sensor_analyzer_optimized: analisa_log_IoT_otim.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS)
