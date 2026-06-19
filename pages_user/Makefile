# Compilacion de las aplicaciones del usuario para pruebas sobre paginas
CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -march=native

SRCS = $(wildcard *.c)

TARGETS = $(patsubst %.c,%,$(SRCS))

all: $(TARGETS)

%: %.c
	$(CC) -o $@ $< $(CFLAGS)

clean:
	rm -f $(TARGETS)
