/* THP - TRANSPARENT HUGE PAGES
 *
 * Se piden bloques de 2MB alineados a 2MB y se marcan con
 * madvise(MADV_HUGEPAGE) para que el kernel use paginas enormes.
 *
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#define HUGE_PAGE_SIZE (2 * 1024 * 1024)            // Tamaño de huge page: 2MB
#define BLOCK_SIZE HUGE_PAGE_SIZE                   // Bloques de 2MB
#define MAX_ALLOCATE (1ULL * 1024 * 1024 * 1024)    // GB
#define MAX_BLOCKS (MAX_ALLOCATE / BLOCK_SIZE + 1)  // Tamaño máximo del arreglo de punteros

int main() {
    size_t total_allocated = 0;
    srand(time(NULL));

    // Arreglo para almacenar los punteros asignados
    int *allocated_blocks[MAX_BLOCKS];
    int block_count = 0;

    printf("Iniciando consumo de RAM en bloques de 2MB (THP)\n");
    while (total_allocated < MAX_ALLOCATE) {
        int *ptr = NULL;

        // Memoria alineada a 2MB, requisito para que se pueda usar una huge page
        if (posix_memalign((void **)&ptr, HUGE_PAGE_SIZE, BLOCK_SIZE) != 0) {
            printf("\nNo se pudo asignar más memoria\n");
            break;
        }

        // Pedir al kernel que use THP para este bloque (antes de tocar la memoria)
        if (madvise(ptr, BLOCK_SIZE, MADV_HUGEPAGE) != 0) {
            perror("\nmadvise(MADV_HUGEPAGE)");
            free(ptr);
            break;
        }

        // Al escribir se producen los page faults y el kernel asigna la huge page
        for (size_t i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            ptr[i] = rand();
        }

        // Guardar el puntero en el arreglo
        allocated_blocks[block_count] = ptr;
        block_count++;

        total_allocated += BLOCK_SIZE;
        printf("\rConsumo actual: %zu MB", total_allocated / (1024 * 1024));
        fflush(stdout);
    }

    printf("\nTHP: Esperando 3 segundos antes de liberar...\n");
    sleep(3);

    // Liberar la memoria iterando sobre los punteros guardados
    for (int i = 0; i < block_count; i++) {
        free(allocated_blocks[i]);
    }
    printf("Memoria liberada correctamente.\n\n");

    return 0;
}
