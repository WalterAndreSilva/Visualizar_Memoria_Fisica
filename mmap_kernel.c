#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#include <linux/atomic.h>
#include <linux/types.h>

#define RAM_SIZE_GB 16ULL
#define MAX_PHYS_ADDR (RAM_SIZE_GB * 1024 * 1024 * 1024)
#define MAX_PFN (MAX_PHYS_ADDR >> PAGE_SHIFT)

// (16*1024*1024*1024)/4096 = 2048*2048 = 4194304
#define MAP_SIZE (2048 * 2048) // 4.19 MB exactos para nuestra textura
#define PAGES_NUMBER (MAP_SIZE / PAGE_SIZE) // Exactamente 1024 páginas

static const char *filename = "lkmc_mmap";

// Estructura global para clasificar toda la RAM
struct ram_mapping {
    uint32_t *valid_pfns;
    unsigned long valid_count;

    uint32_t *invalid_pfns;
    unsigned long invalid_count;
};

static struct ram_mapping map_data;

// Añadimos el hilo y el contador a nuestra estructura mmap
struct mmap_info {
    uint8_t *data;
    struct task_struct *thread;
    atomic_t refcnt; 
};

// Función de limpieza segura
static void mmap_info_put(struct mmap_info *info)
{
    // Solo liberamos cuando la referencia llega a 0
    if (atomic_dec_and_test(&info->refcnt)) {
        if (info->data) {
            vfree(info->data);
        }
        kfree(info);
    }
}

static void vm_open(struct vm_area_struct *vma)
{
    struct mmap_info *info = vma->vm_private_data;
    pr_info("vm_open\n");
    atomic_inc(&info->refcnt); // Incrementamos referencia al mapear
}

static void vm_close(struct vm_area_struct *vma)
{
    struct mmap_info *info = vma->vm_private_data;
    pr_info("vm_close\n");
    mmap_info_put(info); // Decrementamos al desmapear
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
    struct mmap_info *info = (struct mmap_info *)vmf->vma->vm_private_data;
    struct page *page;

    if (vmf->pgoff >= PAGES_NUMBER) {
        return VM_FAULT_SIGBUS;
    }

    // Usamos vmalloc_to_page porque info->data fue creado con vmalloc
    page = vmalloc_to_page(info->data + (vmf->pgoff << PAGE_SHIFT));
    if (!page) {
        return VM_FAULT_SIGBUS;
    }

    get_page(page);
    vmf->page = page;
    // pr_info("vm_fault: entregando pagina: %lu\n",vmf->pgoff);
    return 0;
}

static struct vm_operations_struct vm_ops =
{
    .close = vm_close,
    .fault = vm_fault,
    .open = vm_open,
};

static int mmap(struct file *filp, struct vm_area_struct *vma)
{
    pr_info("mmap\n");
    vma->vm_ops = &vm_ops;
    vm_flags_set(vma, VM_DONTEXPAND | VM_DONTDUMP);
    vma->vm_private_data = filp->private_data;
    vm_open(vma);
    return 0;
}

static inline uint8_t get_value(unsigned char *page_ptr)
{
    uint64_t *p = (uint64_t *)page_ptr;
    uint64_t v0 = 0, v1 = 0, v2 = 0, v3 = 0;

    for (int j = 0; j < 512; j += 8) {
        v0 ^= p[j]   ^ p[j+1];
        v1 ^= p[j+2] ^ p[j+3];
        v2 ^= p[j+4] ^ p[j+5];
        v3 ^= p[j+6] ^ p[j+7];
    }

    uint64_t value_64 = v0 ^ v1 ^ v2 ^ v3;

    value_64 ^= (value_64 >> 32);
    value_64 ^= (value_64 >> 16);
    value_64 ^= (value_64 >> 8);

    return (uint8_t)(value_64 & 0xFF);

    // si retorno el primer elemento anda 12 veces mas rapido
    //uint8_t *p = (uint8_t *)page_ptr;
    //return p[0];
}

static int update_data_thread(void *data)
{
    struct mmap_info *info = (struct mmap_info *)data;
    uint8_t iteration = 0;
    unsigned long next_second = jiffies + HZ;
    uint8_t iteration_per_second = 0;

    while (!kthread_should_stop()) {
        info->data[0] = iteration_per_second;
        for (unsigned long i = 0; i < map_data.valid_count; i++) {
            uint32_t pfn = map_data.valid_pfns[i];
            unsigned char *page_ptr = __va((unsigned long)pfn << PAGE_SHIFT);

            info->data[pfn] = get_value(page_ptr);

            // Ceder control (relentiza pero sino se tilda)
            if ((i % 524288) == 0) cond_resched();
        }
        // Calculo de performance
        iteration ++;
        if(time_after(jiffies, next_second)){
            //pr_info("actualizaciones por segundo: %hhu", iteration);
            iteration_per_second = iteration;
            iteration = 0;
            next_second = jiffies + HZ;
        }
    }
    return 0;
}

// El escaneo de RAM se realiza en la carga del módulo (se hace solo una vez)
static int scan_and_store_ram(void)
{
    unsigned long pfn;
    unsigned long valid_count = 0;
    unsigned long invalid_count = 0;

    pr_info("Contando PFNs válidos e inválidos...\n");
    // evito la pagina 0 (envio de info)
    for (pfn = 1; pfn < MAX_PFN; pfn++) {
        if (page_is_ram(pfn)) valid_count++;
        else invalid_count++;

        if ((pfn % 1024) == 0) cond_resched();
    }

    map_data.valid_pfns = vmalloc(valid_count * sizeof(uint32_t));
    map_data.invalid_pfns = vmalloc(invalid_count * sizeof(uint32_t));

    if (!map_data.valid_pfns || !map_data.invalid_pfns) {
        pr_err("Error: No hay memoria para las estructuras de mapeo\n");
        if (map_data.valid_pfns) vfree(map_data.valid_pfns);
        if (map_data.invalid_pfns) vfree(map_data.invalid_pfns);
        return -ENOMEM;
    }

    map_data.valid_count = 0;
    map_data.invalid_count = 0;

    pr_info("Poblando estructuras de PFNs...\n");
    for (pfn = 1; pfn < MAX_PFN; pfn++) {
        if (page_is_ram(pfn)) {
            map_data.valid_pfns[map_data.valid_count] = pfn;
           // map_data.valid_pages[map_data.valid_count].page_ptr = __va(pfn << PAGE_SHIFT);
            map_data.valid_count++;
        } else {
            map_data.invalid_pfns[map_data.invalid_count] = pfn;
            map_data.invalid_count++;
        }

        if ((pfn % 1024) == 0) cond_resched();
    }
    pr_info("Escaneo completo. Válidas: %lu, Inválidas: %lu\n",
            map_data.valid_count, map_data.invalid_count);
    return 0;
}

static int open(struct inode *inode, struct file *filp)
{
    struct mmap_info *info;
    pr_info("open\n");
    info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    if (!info) return -ENOMEM;

    // Reservamos los 4.19 MB virtualmente contiguos
    info->data = vmalloc_user(MAP_SIZE);
    if (!info->data) {
        pr_err("Fallo al reservar memoria para el buffer\n");
        kfree(info);
        return -ENOMEM;
    }
    atomic_set(&info->refcnt,1);
    filp->private_data=info;

    // Inicializar páginas no disponibles UNA SOLA VEZ
    for (unsigned long i = 0; i < map_data.invalid_count; i++) {
        uint32_t idx = map_data.invalid_pfns[i];
        info->data[idx] = (uint8_t)255;
    }
    info->thread = kthread_run(update_data_thread, info, "mmap_gen_thread");
    return 0;
}

static ssize_t read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    struct mmap_info *info;
    ssize_t ret;

    pr_info("read\n");
    if ((size_t)MAP_SIZE <= *off) {
        ret = 0;
    } else {
        info = filp->private_data;
        ret = min(len, (size_t)MAP_SIZE - (size_t)*off);
        if (copy_to_user(buf, info->data + *off, ret)) {
            ret = -EFAULT;
        } else {
            *off += ret;
        }
    }
    return ret;
}

static ssize_t write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    struct mmap_info *info;

    pr_info("write\n");
    info = filp->private_data;
    if (copy_from_user(info->data, buf, min(len, (size_t)MAP_SIZE))) {
        return -EFAULT;
    } else {
        return len;
    }
}

static int release(struct inode *inode, struct file *filp)
{
    struct mmap_info *info = filp->private_data;
    pr_info("release\n");

    // Detenemos el hilo específico de esta conexión
    if (info && info->thread) {
        kthread_stop(info->thread);
        info->thread = NULL;
    }
    // Soltamos la referencia del File Descriptor
    if (info) {
        mmap_info_put(info);
    }
    filp->private_data = NULL;
    return 0;
}

static const struct proc_ops fops = {
    .proc_mmap = mmap,
    .proc_open = open,
    .proc_release = release,
    .proc_read = read,
    .proc_write = write,
};

static int myinit(void)
{
    int ret = scan_and_store_ram();
    if (ret) return ret;
    
    proc_create(filename, 0, NULL, &fops);
    return 0;
}

static void myexit(void)
{
    remove_proc_entry(filename, NULL);
    // Se libera la memoria global al descargar el módulo
    if (map_data.valid_pfns) {
        vfree(map_data.valid_pfns);
        map_data.valid_pfns = NULL;
    }

    if (map_data.invalid_pfns) {
        vfree(map_data.invalid_pfns);
        map_data.invalid_pfns = NULL;
    }
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
