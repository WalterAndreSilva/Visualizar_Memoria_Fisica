/* ANONIMA / USUARIO
 *
 * Reserva memoria en bloques de N paginas hasta saturar la RAM.
 * Al usar malloc con bloques grandes, glibc usa mmap anonimo, por lo que
 * los bloques quedan alineados a pagina y se marcan como paginas anonimas.
 *
 * ADVERTENCIA: puede congelar el sistema (swap/thrashing) o disparar el
 * OOM killer. Ejecutar solo en una VM o equipo de pruebas.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PAGES_PER_BLOCK 256   // 256 paginas de 4 KB = 1 MB por bloque
#define REPORT_EVERY    64    // Informar cada 64 bloques (64 MB)

// Lista enlazada: el nodo vive dentro del propio bloque, sin arreglo aparte
typedef struct block {
    struct block *next;
} block_t;

int main(void) {
    long page_size = sysconf(_SC_PAGESIZE);
    size_t block_size = (size_t)PAGES_PER_BLOCK * page_size;
    size_t total_allocated = 0;
    size_t block_count = 0;
    block_t *head = NULL;

    srand(time(NULL));

    printf("Tamaño de pagina: %ld bytes\n", page_size);
    printf("Iniciando consumo de RAM en bloques de %zu KB (%d paginas)\n", block_size / 1024, PAGES_PER_BLOCK);

    while (1) {
        block_t *b = malloc(block_size);
        if (b == NULL) {
            printf("\nNo se pudo asignar mas memoria (malloc devolvio NULL)\n");
            break;
        }

        for (size_t p = 0; p < PAGES_PER_BLOCK; p++) {
            memset((char *)b + p * page_size, (rand() % 255) + 1, page_size);
        }

        b->next = head;
        head = b;

        block_count++;
        total_allocated += block_size;

        if (block_count % REPORT_EVERY == 0) {
            printf("\rConsumo actual: %zu MB", total_allocated / (1024 * 1024));
            fflush(stdout);
        }
    }

    return 0;
}
