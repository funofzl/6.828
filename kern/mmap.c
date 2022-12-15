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

extern void user_mem_assert(struct Env *env, const void *va, size_t len, int perm);

// allocate a page to store mmap_region info.
struct MmapRegion mmap_regions[MMAP_NUMS]; // default: 1024

// 先写死 每二个mmap最大为1MB 所需要的映射地址为
// 或者可以修改为每次去global 查找其他的mmap_region信息 判断是否有交集 然后拿到物理地址
uint32_t mmap_paddrs[MMAP_NUMS][MAX_MMAP_SIZE/PGSIZE];

void setRegionNull(uint32_t i) {
	mmap_regions[i].fd = -2;
}

bool regionIsNull(uint32_t i) {
	return mmap_regions[i].fd == -2;
}

void mmap_region_init() {
	for(uint32_t i=0;i<MMAP_NUMS;i++) {
		setRegionNull(i);
	}
}

uint32_t getAvaMmapRegion() {
	for(uint32_t i=0;i<MMAP_NUMS;i++) {
		if(regionIsNull(i)) {
			return i;
		}
	}
	return -1;
}

// Get n bytes from mmap gion.
// TODO: Add list struct later.
void* AllocNBytes(size_t len) {
	len = ROUNDUP(len, PGSIZE); // perhaps is not necessary.
	void *mmap_cur = curenv->mmap_cur;
	if(mmap_cur + len > (void*)MMAP_END) {
		return NULL;
	}
	curenv->mmap_cur += len;
	return mmap_cur;
}


/*
	Only the flags's MAP_SHARED is set, we should call checkExists, or just allocate new space.
*/
/*
bool checkExists(int fd, off_t offset, off_t end) {
	for(int i=0;i<MMAP_NUMS;i++) {
		struct MmapRegion* tmp = regions[i];
		if(tmp != NULL) {
			if(fd == tmp->fd) {
				// 1. if the perm matchs.(shared not private)
				if(!(fd->flags & MAP_SHARED)) {
					continue;
				}
				// 2. if the address has join space.
				uint32_t maxLeft = max(tmp->offset, offset);
				uint32_t minRight = min(tmp->end, end);
				if(maxLeft >= minRight)	{
					// does not have common space.
					continue;
					// TODO:
				}
				
			}
		}
	}	
	return false;
}
*/

/* 
	1.  Just alloc the virtual space, not physical space.
	1.1 Check if the va address valid:(perm, address)

	2.	Find a available mmap_region to store the mmap region info.
		though same process mmap in same fd and same offset, the va is diff.
	3.	Set the mmap_rgion info.
 */
void *
my_mmap(void* va, size_t len, int prot, int flags, int fd, off_t offset) {
	// 1 Check ......
	assert((offset & (PGSIZE-1)) == 0); // the offset should be a multiple of page size.
	
	int perm = PTE_P | PTE_U;
	if(!(prot & PROT_READ)) {
		cprintf("Prot should at least be PROT_READ\n");
		return NULL;
	}
	if(prot & PROT_WRITE) {
		perm |= PTE_W;
	}

	uint32_t real_offset = ROUNDDOWN(offset, PGSIZE);
	uint32_t real_end = ROUNDUP(offset + len, PGSIZE);
	uint32_t real_len = real_end - real_offset;
	// check if reach the MAX_MMAP_SIZE
	if(real_end > MAX_MMAP_SIZE) {
		panic("in my_mmap: reach the MAX_MMAP_SIZE\n");
	}
	if(va != NULL) {
		// TODO: 先做va == nullptr的工作
		user_mem_assert(curenv, va, len, perm);
	} else {
		va = AllocNBytes(real_len);
		if(va == NULL) {
			panic("Out of memory(virtual address)\n");
		}
	}

	// 2. Find a null mmap_region 
	uint32_t i = getAvaMmapRegion();
	if(i == -1) {
		panic("in mmap: Does not has avaiaable mmap_region.\n");
	}
	//   Initialize paddrs to zero.
	memset(mmap_paddrs, 0, sizeof(uint32_t) * MAX_MMAP_SIZE / PGSIZE);

	// 3. Set the mmap_region info.
	assert(fd >= -1);	// fd == -1 (anonymous)
	mmap_regions[i].fd = fd;
	mmap_regions[i].flags = flags; // real prot | real flags
	mmap_regions[i].offset = real_offset;
	mmap_regions[i].end = real_end;
	mmap_regions[i].va = (uint32_t)va;
	mmap_regions[i].next = curenv->mmap_list;
	curenv->mmap_list = i;

	// 并不直接读取文件 而是等使用的时候 产生page fault
	// 在page fault中判断是mmap page fault然后 分配物理内存
	return va;
}


int my_munmap(void *addr, size_t len) {
	// 1. Ensure the addr is a multiple of PGSIZE.
	if(PGOFF(addr) != 0) {
		return -1;
	}

	// 2. calculate all the begin page and end boundary.
	uint32_t begin_addr = (uint32_t)addr;
	uint32_t end_addr = (uint32_t)ROUNDUP(addr + len, PGSIZE);
	//     Ensure [addr, addr+len) belongs to [MMAP_START, MMAP_END);
	if(!( begin_addr >= MMAP_START && end_addr <= MMAP_END )) {
		return -E_INVAL; // range 不在mmap 范围内
	}

	// 3. Find the corresponding MmapRegion of this env for every page.
	uint32_t curaddr = begin_addr;
	while(curaddr < end_addr) {
		// 3.1. If this env does not has mmap for curaddr, just skip.
		struct PageInfo *pp = page_lookup(curenv->env_pgdir, (void*)curaddr, NULL);
		if(pp == NULL) {
			curaddr += PGSIZE;
			continue;
		}
		// 不能简单的page_remove() -> decref()就行了 还需要检查是不是在某个mmap_region里
		uint32_t i = curenv->mmap_list;
    	for(; i!=-1; i++) {
    	    size_t length = mmap_regions[i].end - mmap_regions[i].offset;
    	    if(mmap_regions[i].va <= (uint32_t)addr && (uint32_t)addr < mmap_regions[i].va + length) {
    	        break;
    	    }
    	    i = mmap_regions[i].next;
    	}   
		// If does not found, just skip.
		// TODO: 后期再修改吧 因为这里va不会重复
		if(i != -1) {
			page_remove(curenv->env_pgdir, (void*)curaddr);
			if(pp->pp_ref == 0) {  // pp_ref == mmap count for this page.
				size_t pagenum = ((uint32_t)curaddr - mmap_regions[i].va + mmap_regions[i].offset) / PGSIZE;
				int fd = mmap_regions[i].fd;
				mmap_paddrs[fd][pagenum] = 0;
				// 在最后ref == 0的时候写入磁盘吗
			}
		}
		curaddr += PGSIZE;
	}
	return 0;
}





