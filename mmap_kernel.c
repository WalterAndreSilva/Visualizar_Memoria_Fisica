#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>    // min
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>   // copy_from_user, copy_to_user
#include <linux/slab.h>      // Para kmalloc
#include <linux/delay.h>     // msleep()
#include <linux/kthread.h>
#include <linux/io.h>        // memremap()

#define PAGES_NUMBER 128     // 128* 4k = 512k
#define EXP_RESERVE 7        // 2^EXP_RESERVE = PAGES_NUMBER

enum { BUFFER_SIZE = 480000 };

static const char *filename = "lkmc_mmap";

static struct task_struct *update_thread;
struct mmap_info *global_info;

struct mmap_info {
    char *data;
};

static void vm_close(struct vm_area_struct *vma)
{
    pr_info("vm_close\n");
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
    struct mmap_info *info;
    struct page *page;
    unsigned long address;

    info = (struct mmap_info *)vmf->vma->vm_private_data;
    if (vmf->pgoff >= PAGES_NUMBER) {
        return VM_FAULT_SIGBUS;
    }
    address = (unsigned long)info->data;
    page = virt_to_page(address + (vmf->pgoff << PAGE_SHIFT));
    get_page(page);
    vmf->page = page;
    //pr_info("vm_fault: entregando página %lu\n", vmf->pgoff);
    return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
    pr_info("vm_open\n");
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

static int read_data(char *data)
{
//   /*
    unsigned long base = 0x000000;      // Dirección física a leer
    size_t size = 4096;                 // Cuántos bytes leer
    void __iomem *virtual_addr;         // Puntero virtual
    char *cache;                        // Datos leidos de la memoria fisica

    cache = kmalloc(size, GFP_KERNEL);
    if (!cache)
        return -ENOMEM;

    unsigned long phys_addr = base;
    for (int i=0; i<BUFFER_SIZE; i++){
        virtual_addr = memremap(phys_addr, size, MEMREMAP_WB);
        if (!virtual_addr) {
            printk(KERN_ERR "Error: No se pudo mapear la memoria física\n");
            return -ENOMEM;
        }
        memcpy(cache, virtual_addr, size);
        char value = 0;
        for(int j=0; j<size; j++){
            value ^= cache[j];
        }
        data[i] = value;
        memunmap(virtual_addr);
        phys_addr += size;
    }
    kfree(cache);
//    */

    // random pixel
    /*
    for (int i=0; i<BUFFER_SIZE; i++){
        data[i] = get_random_u8();
    }
    */
    return 0;
}

static int random_generator_thread(void *data)
{
    struct mmap_info *info = (struct mmap_info *)data;
    char *data_buf;
    data_buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!data_buf)
        return -ENOMEM;

    while (!kthread_should_stop()) {
        read_data(data_buf);
        for (int i = 0; i < BUFFER_SIZE; i++) {
            info->data[i] = data_buf[i];
        }
        msleep(50); // Actualiza cada 50ms
    }
    kfree(data_buf);
    return 0;
}

static int open(struct inode *inode, struct file *filp)
{
    struct mmap_info *info;

    pr_info("open\n");
    info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
    info->data = (char *)__get_free_pages(GFP_KERNEL | __GFP_ZERO, EXP_RESERVE); // 2^EXP_RESERVE = paginas reservadas
    filp->private_data = info;

    global_info = info;
    update_thread = kthread_run(random_generator_thread, info, "mmap_gen_thread");

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
    struct mmap_info *info;

    pr_info("release\n");
    info = filp->private_data;
    free_pages((unsigned long)info->data, EXP_RESERVE);
    kfree(info);
    filp->private_data = NULL;

    if (update_thread) {
        kthread_stop(update_thread);
    }

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
    proc_create(filename, 0, NULL, &fops);
    return 0;
}

static void myexit(void)
{
    remove_proc_entry(filename, NULL);
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");
