#ifndef JOS_KERN_MMAP_H
#define JOS_KERN_MMAP_H

#include <inc/mmap.h>

#define MMAP_START          0x11000000
#define MMAP_END            0x12000000
#define MMAP_NUMS           128 //因为进入kernnel时设置的entry_pgdir只到0xf03ff000 所以设置的太多会导致后面启动失败
#define MAX_MMAP_SIZE       0x100000  // 1MB 1 << 20
#define MAX_MMAP_PAGE_NUM   MAX_MMAP_SIZE/PGSIZE

extern struct MmapRegion mmap_regions[MMAP_NUMS];

extern uint32_t mmap_paddrs[MMAP_NUMS][MAX_MMAP_PAGE_NUM];

// ----------------------------

void mmap_region_init();

void *my_mmap(void* va, size_t len, int prot, int flags, int fd, off_t offset);

int my_munmap(void *addr, size_t len);

#endif
