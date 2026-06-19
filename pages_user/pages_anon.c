/* ANONIMA / USUARIO
 *
 * Al usar malloc se marcan las paginas como anonimas
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>

#define BLOCK_SIZE (1 * 1024 * 1024)                // Bloques MB
#define MAX_ALLOCATE (4ULL * 1024 * 1024 *1024)     // GB
#define MAX_BLOCKS (MAX_ALLOCATE / BLOCK_SIZE + 1)  // Tamaño máximo del arreglo de punteros

int main() {
    size_t total_allocated = 0;
    srand(time(NULL));

    // Arreglo para almacenar los punteros asignados
    int *allocated_blocks[MAX_BLOCKS];
    int block_count = 0;

    printf("Iniciando consumo de RAM en bloques de 1MB\n");
    while(total_allocated < MAX_ALLOCATE){
        int *ptr = malloc(BLOCK_SIZE);
        
        if (ptr == NULL) {
            printf("No se pudo asignar más memoria\n");
            break;
        }

        for (size_t i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            ptr[i] = rand();
        }

        // Guardar el puntero en el arreglo
        allocated_blocks[block_count] = ptr;
        block_count++;

        total_allocated += BLOCK_SIZE;
        printf("\rConsumo actual: %zu MB", total_allocated / (1024 * 1024));
    }

    printf("\nPAGINAS ANONIMAS: Presiona Enter para liberar y salir...");
    getchar();

    // Liberar la memoria iterando sobre los punteros guardados
    for (int i = 0; i < block_count; i++) {
        free(allocated_blocks[i]);
    }
    printf("Memoria liberada correctamente.\n\n");

    return 0;
}
