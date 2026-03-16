#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BLOCK_SIZE 100 * 1024 * 1024 // Bloques de 100 MB

int main() {
    size_t total_allocated = 0;
    srand(time(NULL));

    printf("Iniciando consumo de RAM...\n");
    // LLena memoria hasta fallar.
    // Tambien se puede limitar el tamaño de memoria a llenar
    while(1){
        int *ptr = malloc(BLOCK_SIZE);

        if (ptr == NULL) {
            printf("\nNo se pudo asignar más memoria. Límite alcanzado.\n");
            break;
        }

        for (size_t i = 0; i < BLOCK_SIZE / sizeof(int); i++) {
            ptr[i] = rand();
        }

        total_allocated += BLOCK_SIZE;
        printf("Consumo actual: %zu MB\n", total_allocated / (1024 * 1024));

        // Pequeña pausa
        usleep(500000);
    }

    printf("Manteniendo memoria ocupada. Presiona Ctrl+C para salir.\n");
    while(1) sleep(1);

    return 0;
}
