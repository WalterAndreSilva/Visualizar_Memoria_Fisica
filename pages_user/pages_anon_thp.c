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

#define SIZE_MB (1024 * 1024)
#define MAX_ALLOCATE (4ULL * 1024 * 1024 *1024)        // GB

void misaligned_memory(int b_size){
    size_t total_allocated = 0;
    unsigned int block_size = b_size * SIZE_MB;
    int max_blocks = (MAX_ALLOCATE / block_size + 1);
    srand(time(NULL));

    // Arreglo para almacenar los punteros asignados
    int *allocated_blocks[max_blocks];
    int block_count = 0;

    printf("Iniciando consumo de RAM en bloques de %dMB\n", b_size);
    while(total_allocated < MAX_ALLOCATE){
        int *ptr = malloc(block_size);
        
        if (ptr == NULL) {
            printf("No se pudo asignar más memoria\n");
            break;
        }

        for (size_t i = 0; i < block_size / sizeof(int); i++) {
            ptr[i] = rand();
        }

        // Guardar el puntero en el arreglo
        allocated_blocks[block_count] = ptr;
        block_count++;

        total_allocated += block_size;
        printf("\rConsumo actual: %zu MB", total_allocated / (1024 * 1024));
    }

    printf("\nPAGINAS ANONIMAS: Presiona Enter para liberar y salir...");
    getchar();

    // Liberar la memoria iterando sobre los punteros guardados
    for (int i = 0; i < block_count; i++) {
        free(allocated_blocks[i]);
    }
    printf("Memoria liberada correctamente.\n\n");
}


void aligned_memory(int b_size){
    size_t total_allocated = 0;
    unsigned int block_size = b_size * SIZE_MB;
    int max_blocks = (MAX_ALLOCATE / block_size + 1);
    srand(time(NULL));

    // Arreglo para almacenar los punteros asignados
    int *allocated_blocks[max_blocks];
    int block_count = 0;

    printf("Iniciando consumo de RAM en bloques de %dMB\n", b_size);
    while(total_allocated < MAX_ALLOCATE){
        int *ptr = mmap(NULL, block_size, PROT_READ | PROT_WRITE, 
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (ptr == NULL) {
            printf("No se pudo asignar más memoria\n");
            break;
        }

        for (size_t i = 0; i < block_size / sizeof(int); i++) {
            ptr[i] = rand();
        }

        // Guardar el puntero en el arreglo
        allocated_blocks[block_count] = ptr;
        block_count++;

        total_allocated += block_size;
        printf("\rConsumo actual: %zu MB", total_allocated / (1024 * 1024));
    }

    printf("\nPAGINAS ANONIMAS: Presiona Enter para liberar y salir...");
    getchar();

    // Liberar la memoria iterando sobre los punteros guardados
    for (int i = 0; i < block_count; i++) {
        munmap(allocated_blocks[i], block_size);
    }
    printf("Memoria liberada correctamente.\n\n");
}

int main() {

    printf("\e[36m\nMEMORIA NO ALINEADA (malloc)\n\n\e[0m");
    for(int i = 1; i <= 4; i++){
        misaligned_memory(i);
    }
    
    printf("\e[36mMEMORIA ALINEADA (mmap)\n\n\e[0m");
    for(int i = 1; i <= 4; i++){
      aligned_memory(i);
    }

    return 0;
}
