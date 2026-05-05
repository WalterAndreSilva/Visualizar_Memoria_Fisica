# Compilacion del modulo del kernenl y las aplicaciones del usuario
CFLAGS = -std=c11 -Wall -Wextra -O2 -march=native

all: pages_anon pages_comp

pages_anon: pages_anon.c
	gcc -o pages_anon pages_anon.c $(CFLAGS)

pages_comp: pages_comp.c
	gcc -o pages_comp pages_comp.c $(CFLAGS)

clean:
	rm -f pages_anon
	rm -f pages_comp
