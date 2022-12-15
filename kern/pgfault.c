#include <inc/x86.h>
#include <inc/mmu.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/kclock.h>
#include <kern/env.h>
#include <kern/cpu.h>

#include <kern/mmap.h>


void mmap_pgfault_handler(void *addr)
{
    int r;
	// TODO: 什么才算是真正的page fault.

	uint32_t i = curenv->mmap_list;
	for(; i!=-1; i++) {
		size_t len = mmap_regions[i].end - mmap_regions[i].offset;
		if(mmap_regions[i].va <= (uint32_t)addr && (uint32_t)addr < mmap_regions[i].va + len) {
			break;
		}
		i = mmap_regions[i].next;
	}
	if(i == -1) {
		panic("in mmap_pgfault_handler: addr(%08x) not in a valid mmap region\n", addr);
	}
	cprintf("kern/pgfault.c: find i:%d\n", i);

	// 只有MAP_SHARED才会共享并且unmap的时候写入磁盘
	// 如果已经有了shared,map_private的时候会将数据拷贝一份
	// 如果先有map_private, 其实就是先先拷贝了一份数据 之后就无关了
	int perm = PTE_P| PTE_U;
	if(mmap_regions[i].flags & PROT_WRITE) {
		perm |= PTE_W;
	}
	if(mmap_regions[i].flags & MAP_PRIVATE) {
		// if is private, just page_alloc and page_insert.
		struct PageInfo *pp = page_alloc(1);
		assert(pp != NULL);
		r = page_insert(curenv->env_pgdir, pp, addr, perm);
		if(r < 0) {
			panic("in mmap_pgfault_handler: page_insert error, %e\n", r);
		}
		// TODO: read page from disk.
	} else {
		cprintf("kern/pgfult.c: MAP_SHARED\n");
		size_t pagenum = ((uint32_t)addr - mmap_regions[i].va + mmap_regions[i].offset) / PGSIZE;
		cprintf("pagenum: %d\n", pagenum);
		if(pagenum >= MAX_MMAP_PAGE_NUM) {
			panic("in mmap_pgfault_handler: Exceed the MAX_MMAP_PAGE_NUM\n");
		}
		int fd = mmap_regions[i].fd;
		uint32_t paddr = mmap_paddrs[fd][pagenum];
		struct PageInfo* pp;
		if(paddr == 0) {
			pp = page_alloc(1);
			assert(pp != NULL);
			// insert into public mmap_paddrs array.
			mmap_paddrs[fd][pagenum] = page2pa(pp);
		} else {
			// get pp via the physaddr
			pp = pa2page(paddr);
		}
		// Add refcount of this page. TODO: BUG up to 1023 references.
		// mmap_paddrs[fd][pagenum]++;
		r = page_insert(curenv->env_pgdir, pp, addr, perm);
		if(r < 0) {
			panic("in mmap_pgfault_handler: page_insert error, %e\n", r);
		}
		// TODO: read page from disk	
	}
}
