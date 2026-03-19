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

#define RAM_SIZE_GB 16ULL
#define MAX_PHYS_ADDR (RAM_SIZE_GB * 1024 * 1024 * 1024)
#define MAX_PFN (MAX_PHYS_ADDR >> PAGE_SHIFT)

// (16*1024*1024*1024)/4096 = 2048*2048 = 4194304
#define MAP_SIZE (2048 * 2048) // 4.19 MB exactos para nuestra textura
#define PAGES_NUMBER (MAP_SIZE / PAGE_SIZE) // Exactamente 1024 páginas

static void **ram_pointers;
static unsigned long valid_pages_count = 0;

static const char *filename = "lkmc_mmap";

// Añadimos el hilo y el contador a nuestra estructura mmap
struct mmap_info {
    char *data;
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
    atomic_inc(&info->refcnt); // Incrementamos referencia al mapear
    pr_info("vm_open\n");
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
    vm_open(vma); // Llama a vm_open que sumará 1 a la referencia
    return 0;
}

static int update_data_thread(void *data)
{
    struct mmap_info *info = (struct mmap_info *)data;
    unsigned long i;
    unsigned char *page_ptr;

    while (!kthread_should_stop()) {
        for (i = 0; i < MAP_SIZE; i++) {
            if (kthread_should_stop()) break;

            page_ptr = (unsigned char *)ram_pointers[i];

            if (page_ptr != NULL) {
                char value = 0;
                for(int j = 0; j < PAGE_SIZE; j++){
                    value ^= page_ptr[j];
                }
                info->data[i] = value;
            } else{
                info->data[i] = (char)255; // blanco -> rojo
            }
            if ((i % 1024) == 0) {
                cond_resched();
            }
        }
        msleep(50); 
    }
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

// El escaneo de RAM se realiza en la carga del módulo (se hace solo una vez)
static int scan_and_store_ram(void)
{
    unsigned long pfn;
    ram_pointers = vmalloc(MAX_PFN * sizeof(void *));
    if (!ram_pointers) {
        pr_err("Error: No hay memoria para arreglo de punteros\n");
        return -ENOMEM;
    }

    pr_info("Escaneando PFNs...\n");
    valid_pages_count = 0;
    for (pfn = 0; pfn < MAX_PFN; pfn++) {
        if (page_is_ram(pfn)) {
            ram_pointers[pfn] = __va(pfn << PAGE_SHIFT);
            valid_pages_count++;
        } else{
            ram_pointers[pfn] = NULL;
        }
        // Ceder la CPU para evitar Soft Lockups
        if ((pfn % 1024) == 0) {
            cond_resched();
        }
    }
    pr_info("Escaneo completo. %lu páginas.\n", valid_pages_count);
    return 0;
}

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
    if (ram_pointers) {
        vfree(ram_pointers);
    }
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
