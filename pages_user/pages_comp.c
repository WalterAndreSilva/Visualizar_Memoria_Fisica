/* COMPUESTA/ ANONIMA / USUARIO
 *
 * Este programa solicita memoria desde el usuario para guardar variables.
 * Se solicita al sistama operativo que use Transparent HugePages
 *
*/
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

#define ALLOC_SIZE (512 * 1024 * 1024)  // Tamaño total 512 MB
#define HUGEPAGE_SIZE (2 * 1024 * 1024) // 2 MB (tamaño típico de HugePage en x86_64)

int main() {
    void *ptr = NULL;

    // Asignar memoria alineada a 2MB
    if (posix_memalign(&ptr, HUGEPAGE_SIZE, ALLOC_SIZE) != 0) {
        perror("Error en posix_memalign");
        return 1;
    }

    // Solicitar al kernel que use Transparent HugePages (Compound Pages)
    if (madvise(ptr, ALLOC_SIZE, MADV_HUGEPAGE) != 0) {
        perror("Error en madvise");
    }

    // Escribiendo en la memoria para forzar la asignación física
    memset(ptr, 1, ALLOC_SIZE);

    printf("PAGINAS COMPUESTAS: Presioná ENTER para liberar la memoria y salir...");
    getchar();

    free(ptr);
    printf("Memoria liberada correctamente.\n");

    return 0;
}
