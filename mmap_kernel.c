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

#define MAX_SCAN_GB 32ULL // Mas de los 16 reales
#define MAX_SCAN_PFN ((MAX_SCAN_GB * 1024 * 1024 * 1024) >> PAGE_SHIFT)

// (16*1024*1024*1024)/4096 = 2048*2048 = 4194304
#define MAP_SIZE (2048 * 2048) // 4.19 MB exactos para nuestra textura
#define PAGES_NUMBER (MAP_SIZE / PAGE_SIZE) // Exactamente 1024 páginas

#define VAL_VOID 255
#define VAL_FREE 235
#define VAL_USER 210

static const char *filename = "lkmc_mmap";

// Estructura global para clasificar toda la RAM
struct ram_mapping {
    uint32_t *valid_pfns;
    unsigned long valid_count;
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

static inline uint8_t get_value(uint32_t pfn)
{
    struct page *page = pfn_to_page(pfn);
    uint8_t value = 0;

    //  Sin referencias : page->_refcount = 0
    if (page_count(page) == 0) value = VAL_FREE;
    // La pagina esta refernciada por alguna tabla del userspace
    else if (page_mapped(page)) value = VAL_USER;

    return value;
}

static int update_data_thread(void *data)
{
    struct mmap_info *info = (struct mmap_info *)data;
    uint8_t iteration = 0;
    unsigned long next_second = jiffies + HZ;
    uint8_t iteration_per_second = 0;

    while (!kthread_should_stop()) {
        info->data[0] = iteration_per_second;
        for (unsigned long i = 1; i < map_data.valid_count; i++) {
            uint32_t pfn = map_data.valid_pfns[i];
            info->data[i] = get_value(pfn);
        }
        cond_resched();
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

    for (pfn = 0; pfn < MAX_SCAN_PFN; pfn++) {
        if (pfn_valid(pfn) && page_is_ram(pfn)) {
            valid_count++;
        }
        if ((pfn % 32768) == 0) cond_resched();
    }

    if (valid_count == 0) {
        pr_err("Error: No se detecto RAM valida.\n");
        return -EINVAL;
    }

    map_data.valid_pfns = vmalloc(valid_count * sizeof(uint32_t));
    if (!map_data.valid_pfns) {
        pr_err("Error: No hay memoria para valid_pfns\n");
        return -ENOMEM;
    }

    map_data.valid_count = 0;
    for (pfn = 1; pfn < MAX_SCAN_PFN; pfn++) {
        if (pfn_valid(pfn) && page_is_ram(pfn)) {
            map_data.valid_pfns[map_data.valid_count] = pfn;
            map_data.valid_count++;
        }
        if ((pfn % 32768) == 0) cond_resched();
    }

    pr_info("Escaneo completo. Total de PFNs: %lu\n", map_data.valid_count);
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


    memset(info->data, VAL_VOID, MAP_SIZE); // todo rojo
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
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
