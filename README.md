netmalloc
============

malloc for a LAN on Linux (TCP server and Universal Virtual Memory)

###Install

    git clone https://github.com/xsyann/netmalloc.git
    make
    sudo insmod netmalloc.ko server=127.0.0.1:12345

###Test

    python server/server.py -l 127.0.0.1 -p 12345
    ./run.sh


###Syscalls

    void *netmalloc(unsigned long size);
    void netfree(void *ptr);

`netmalloc` is inserted at offset `__NR_tuxcall` (_tuxcall_ is Not Implemented)

`netfree` is inserted at offset `__NR_security` (_security_ is Not Implemented)

---------------------------------------
###Memory


![](http://www.xsyann.com/epitech/vma_small.png)

Structures used to represent **areas** and **regions** (pseudocode) :

    struct area_struct
    {
        struct vm_area_struct *vma;
        ulong size;
        ulong free_space;
        region_struct regions[];
    };

    struct region_struct
    {
        bool free;
        ulong size;
        ulong start;
    };

An **area** is always linked to a **VMA** and are the same size.

---------------------------------------
###Algorithm

![](http://www.xsyann.com/epitech/vma.gif?raw=1)

`generic_malloc(size)` (pseudocode) :

    generic_malloc(size)
    {
        create_region(size, pid)
    }

    create_region(pid)
    {
        area = get_area(size, pid)
        add_region(area, size)
    }

    get_area(size, pid)
    {
        if an area with pid is big enough, return area
        else if an area with pid exist but too short, extend the area if possible
        else
            area = Create new area
            area.vma = add_vma(size, pid)
    }

    add_region(area, size)
    {
        if (region = get_free_region(size))
            return region
        else
            add a new region after the last existing region in area
    }

    add_vma(size, pid)
    {
        scan vmas of 'pid' process to find the first free space at least 'size' big,
        (start with an offset to avoid inserting vma at virtual address 0x0000)
    }

    get_free_region(size)
    {
        if a free region.size == size, return the free region
        if a free region.size > size
            split the region to get a region.size == size and a free region
            merge the splitted free region with neighbors
    }


When an **area**/**VMA** is created/extended, the size is always a multiple of `PAGE_SIZE`.

#####Fault Handler

When the fault handler is called, the page corresponding to virtual address is filled (from storage) and mapped in the user address space.
Two page at a time can be mapped in user address space for each process / threads.

A static list keeps track of mapped buffers for each pid and another list keeps track of all stored pages to avoid useless requests to storage.

When the fault handler is called and two pages are already mapped in the user memory of this process, the oldest page is unmapped, stored (to storage) and, then, the requested page is mapped.

---------------------------------------
###Generic_malloc

Generic_malloc provide an interface to abstract the storage mode.

    void generic_free(void *ptr);
    void *generic_malloc(unsigned long size);
    void generic_malloc_init(struct storage_ops *s_ops, void *param);
    void generic_malloc_clean(void);

    struct storage_ops
    {
        int (*init)(void *param);
        int (*save)(pid_t pid, unsigned long address, void *buffer);
        int (*load)(pid_t pid, unsigned long address, void *buffer);
        void (*remove)(pid_t pid, unsigned long address);
        void (*release)(void);
    };

Implementation :

    static struct storage_ops storage_ops = {
        .init = network_init,
        .save = network_save,
        .load = network_load,
        .remove = network_remove,
        .release = network_release
    };

    generic_malloc_init(&storage_ops, "192.168.0.3:12345")
    void *ptr = generic_malloc(4096);
    generic_free(ptr);
    generic_malloc_clean();

---------------------------------------
###Multi-thread handling

In a multi-threaded context, we must be careful because the linux kernel calls the fault handler under a mmap_sem read lock. It means that we can down_read, but not down_write in the fault handler.

If generic_alloc is called and, then, a fault occurs in another thread (before generic_alloc execution), the mmap_sem is down_read (by linux kernel) ; during the generic_alloc execution, the down_write(mmap_sem) (before insert_vm_struct) will block, causing a deadlock.

**Lock before each operations**

    // Kernel Module

    __alloc()
    {
        down_write(mmap_sem);
        [not_safe_operations]
        up_write(mmap_sem);

        [safe_operations]

        down_write(mmap_sem);
        [not_safe_operations]
        up_write(mmap_sem);
    }

    alloc()
    {
        mutex_lock(mutex);
        __alloc();
        mutex_unlock(mutex);
    }

    fault_handler()
    {
        mutex_lock(mutex);
        mutex_unlock(mutex);
    }

    // Linux

    do_page_fault()
    {
        down_read(mmap_sem);
        fault_handler();
        up_read(mmap_sem);
    }

**Multi-thread scenario**

    1. alloc();
    2.     mutex_lock(mutex); (alloc)

    3. do_page_fault();
    4.     down_read(mmap_sem); (do_page_fault)
    5.     [ fault_handler(); (do_page_fault)
    6.         mutex_lock(mutex); (fault_handler) ] -> BLOCK

    7.      __alloc(); (alloc)
    8.          down_write(mmap_sem); (__alloc) -> BLOCK
    = DEADLOCK

---------------------------------------

**Highest Lock**

    // Kernel Module

    __alloc()
    {
        [not_safe_operations]
        [operations]
        [not_safe_operations]
    }

    alloc()
    {
        down_write(mmap_sem);
        mutex_lock(mutex);
        __alloc();
        mutex_unlock(mutex);
        up_write(mmap_sem);
    }

    fault_handler()
    {
        mutex_lock(mutex);
        mutex_unlock(mutex);
    }

    // Linux

    do_page_fault()
    {
        down_read(mmap_sem);
        fault_handler();
        up_read(mmap_sem);
    }

**Multi-thread scenario**

    1. alloc();
    2.     down_write(mmap_sem); (alloc)
    3.     mutex_lock(mutex); (alloc)

    4. do_page_fault();
    5.     down_read(mmap_sem); (do_page_fault) -> BLOCK

    6.     __alloc(); (alloc)
    7.         [not_safe_operations] (__alloc)
    8.         [safe_operations] (__alloc)
    9.         [not_safe_operations] (__alloc)
    10.     mutex_unlock(mutex); (alloc)
    11.     up_write(mmap_sem); (alloc)

    12.     fault_handler(); (do_page_fault)
    13.     up_read(mmap_sem); (do_page_fault)
    = GOOD

The vm operations close handler can't be locked because it is called by do_munmap (when a VMA is removed) in the generic_free function which is locked.
