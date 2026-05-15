// huge
#include <stdio.h>
#include <sys/mman.h>

#define HUGE_SIZE (2 * 1024 * 1024)  // 2MB por hugepage
#define NUM_PAGES 6

int main(void)
{
    size_t size = HUGE_SIZE * NUM_PAGES;

    char *mem = mmap(NULL, size,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                     -1, 0);

    if (mem == MAP_FAILED) {
        perror("hugetlb failed");
        printf("usar: sudo sh -c \"echo %d > /proc/sys/vm/nr_hugepages\"", NUM_PAGES);
        return 1;
    }

    // Tocar cada hugepage para forzar asignación
    for (size_t i = 0; i < size; i += HUGE_SIZE) {
        mem[i] = 1;
    }

    printf("HugeTLB: %d paginas de 2MB asignadas (%.0f MB total)\n",
           NUM_PAGES, (double)size / 1024 / 1024);
    printf("Presiona Enter para liberar...\n");
    getchar();

    munmap(mem, size);
    return 0;
}
