# Compilacion de las aplicaciones del usuario para pruebas sobre paginas
CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -march=native

TARGETS = pages_anon pages_huge pages_thp

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGETS)
