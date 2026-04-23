#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/vmalloc.h>
#include "share.h"

#define MAX_SCAN_PFN ((MAX_SCAN_GB * 1024 * 1024 * 1024) >> PAGE_SHIFT)

static const char *filename = "ku_mmap";

// Estructura global para clasificar toda la memoria fisica
struct ram_mapping {
    unsigned long valid_count;

    uint32_t *sysRAM_pfns;
    uint32_t *sysRAM_pos;
    unsigned long sysRAM_count;
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

    if (vmf->pgoff >= BUFFER_SIZE / PAGE_SIZE) {
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

static inline uint8_t get_value_use(uint32_t pfn, uint8_t view_page)
{
    uint8_t value = VAL_UNKN;
    struct page *page = pfn_to_page(pfn);

    //  Sin referencias : page->_refcount = 0
    if (page_count(page) == 0){
        if(view_page & MASK_FREE) value = VAL_FREE;

    } else if (folio_test_pgtable(page_folio(page))){
        if(view_page & MASK_PGTB) value = VAL_PGTB;

    } else if (PageReserved(page)){
        if (view_page & MASK_RESE) value = VAL_RESE;

    } else if (PageCompound(page)) {
        if(view_page & MASK_COMP) value = VAL_COMP;

    // --- USERSPACE ----
    } else if (folio_test_lru(page_folio(page))) {
        if (folio_test_anon(page_folio(page))){
            if(view_page & MASK_ANON) value = VAL_ANON;

        } else {
            if(view_page & MASK_FILE) value = VAL_FILE;
        }
    } else if (page_mapped(page)){
        if(view_page & MASK_USER) value = VAL_USER;
    }else{
        if(view_page & MASK_KERN) value = VAL_KERN;
    }

    return value;

}

static inline uint8_t get_value_zone(uint32_t pfn)
{
    uint8_t value = VAL_UNKN;
    struct page *page = pfn_to_page(pfn);
    // enum zone_type en linux/mmzone.h
    int zone_idx = page_zonenum(page);

    if (zone_idx == ZONE_DMA) value = VAL_ZONE_DMA;
    else if (zone_idx == ZONE_DMA32) value = VAL_ZONE_DMA32;
    else if (zone_idx == ZONE_NORMAL) value = VAL_ZONE_NORMAL;
    else if (zone_idx == ZONE_DEVICE) value = VAL_DIRTY;

    return value;
}

static inline uint8_t get_value_state(uint32_t pfn)
{
    uint8_t value = VAL_UNKN;
    struct page *page = pfn_to_page(pfn);

    if (PageWriteback(page)) value = VAL_WRITEBACK;
    else if (PageDirty(page)) value = VAL_DIRTY;

    return value;
}

static int update_data_thread(void *data)
{
    struct mmap_info *info = (struct mmap_info *)data;
    uint8_t iteration = 0;
    unsigned long next_second = jiffies + HZ;
    uint8_t view_mode = 0;
    uint8_t view_page = MASK_ALL;
    unsigned long total_pages = map_data.valid_count;

    memset(info->data, VAL_VOID, total_pages); // todo rojo
    info->data[INDEX_MODE] = view_mode;
    info->data[INDEX_VIEW] = view_page;
    for (int i = 0; i < 8; i++) {
        info->data[INDEX_TOTAL_PAGES+i] = (total_pages >> (i*8)) & 0xFF;
    }

    while (!kthread_should_stop()) {
        if (view_mode == 0){
            for (unsigned long i = 0; i < map_data.sysRAM_count; i++) {
                uint32_t pfn = map_data.sysRAM_pfns[i];
                uint32_t pos = map_data.sysRAM_pos[i];
                view_page = info->data[INDEX_VIEW];
                info->data[pos] = get_value_use(pfn, view_page);
            }
        } else if (view_mode == 1){
            for (unsigned long i = 0; i < map_data.sysRAM_count; i++) {
                uint32_t pfn = map_data.sysRAM_pfns[i];
                uint32_t pos = map_data.sysRAM_pos[i];
                info->data[pos] = get_value_zone(pfn);
            }
        } else if (view_mode == 2){
            for (unsigned long i = 0; i < map_data.sysRAM_count; i++) {
                uint32_t pfn = map_data.sysRAM_pfns[i];
                uint32_t pos = map_data.sysRAM_pos[i];
                info->data[pos] = get_value_state(pfn);
            }
        }

        cond_resched();
        view_mode = info->data[INDEX_MODE];
        // Calculo de performance
        iteration ++;
        if(time_after(jiffies, next_second)){
            //pr_info("actualizaciones por segundo: %hhu", iteration);
            info->data[INDEX_AKPS] = iteration;
            iteration = 0;
            next_second = jiffies + HZ;
        }
    }
    return 0;
}

// El escaneo de RAM se realiza una vez, en la carga del módulo
static int scan_and_store_ram(void)
{
    unsigned long pfn;
    unsigned long valid_count = 0;

    for (pfn = 0; pfn < MAX_SCAN_PFN; pfn++) {
        if (page_is_ram(pfn)) {
            valid_count++;
        }
        if ((pfn % 32768) == 0) cond_resched();
    }

    if (valid_count == 0) {
        pr_err("Error: No se detecto RAM valida.\n");
        return -EINVAL;
    }

    map_data.sysRAM_pfns = vmalloc(valid_count * sizeof(uint32_t));
    map_data.sysRAM_pos = vmalloc(valid_count * sizeof(uint32_t));
    if (!map_data.sysRAM_pfns) {
        pr_err("Error: No hay memoria para sysRAM_pfns\n");
        return -ENOMEM;
    }
    if (!map_data.sysRAM_pos) {
        pr_err("Error: No hay memoria para sysRAM_pos\n");
        return -ENOMEM;
    }

    map_data.valid_count = 0;
    map_data.sysRAM_count = 0;
    for (pfn = 0; pfn < MAX_SCAN_PFN; pfn++) {
        if (pfn_valid(pfn)){
            if (page_is_ram(pfn)) {
                map_data.sysRAM_pfns[map_data.sysRAM_count] = pfn;
                map_data.sysRAM_pos[map_data.sysRAM_count] = map_data.valid_count;
                map_data.sysRAM_count++;
            }
            map_data.valid_count++;
        }
        if ((pfn % 32768) == 0) cond_resched();
    }

    pr_info("Total PFNs: %lu \n", map_data.valid_count);
    return 0;
}

static int open(struct inode *inode, struct file *filp)
{
    struct mmap_info *info;
    pr_info("open\n");
    info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    if (!info) return -ENOMEM;

    info->data = vmalloc_user(BUFFER_SIZE);
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
    if ((size_t)BUFFER_SIZE <= *off) {
        ret = 0;
    } else {
        info = filp->private_data;
        ret = min(len, (size_t)BUFFER_SIZE - (size_t)*off);
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
    if (copy_from_user(info->data, buf, min(len, (size_t)BUFFER_SIZE))) {
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
    if (map_data.sysRAM_pfns) {
        vfree(map_data.sysRAM_pfns);
        map_data.sysRAM_pfns = NULL;

    }
    if (map_data.sysRAM_pos) {
        vfree(map_data.sysRAM_pos);
        map_data.sysRAM_pos = NULL;

    }
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Módulo para escanear memoria RAM física y mapearla a userspace");
MODULE_AUTHOR("Walter André Silva");
