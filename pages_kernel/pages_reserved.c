#include <linux/module.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/page-flags.h>
#include <linux/random.h>

// Order 10 = 2^10 páginas = 1024 páginas contiguas = 4 MB de RAM.
#define PAGE_ORDER 10

static struct page *pages;

static int __init init_reserva(void)
{
    int total_pages = 1 << PAGE_ORDER;
    unsigned int *ptr_virtual;

    // Pedimos memoria física contigua al kernel
    // Si no se utiliza __GFP_COMP, solo la primera pagina se marca como
    // perteneciente al kernel y el resto como libres
    pages = alloc_pages(GFP_KERNEL | __GFP_COMP, PAGE_ORDER);
    if (!pages) {
        pr_err("Fallo al asignar paginas\n");
        return -ENOMEM;
    }

    for (int i = 0; i < total_pages; i++) {
        // Marcamos la pagina como reservada
        SetPageReserved(&pages[i]);

        ptr_virtual = (unsigned int *)page_address(&pages[i]);

        if (ptr_virtual) {
            *ptr_virtual = get_random_u32();

        }
    }
    pr_info("Paginas reservadas desde el PFN: %lu\n", page_to_pfn(pages));
    return 0;
}

static void __exit exit_reserva(void)
{
    int total_pages = 1 << PAGE_ORDER;

    for (int i = 0; i < total_pages; i++) {
        ClearPageReserved(&pages[i]);
    }

    __free_pages(pages, PAGE_ORDER);
    pr_info("Paginas reservadas liberadas.\n");
}

module_init(init_reserva);
module_exit(exit_reserva);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Generador de paginas reservadas para testing visual");
MODULE_AUTHOR("Walter André Silva");
