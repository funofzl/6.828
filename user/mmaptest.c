#include <inc/lib.h>


void
umain(int argc, char **argv) {
	int fd = 1;
	//void* va = sys_mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	void* va = sys_mmap(NULL, 4096 * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	cprintf("1: va:%08x\n", va);
	assert(va != NULL);
	cprintf("1: old:[%s]\n", va);
	memset(va, 'x', 4096*2);
	*(char*)(va+4096*2-1) = '\0';
	cprintf("1: new:[%s]\n", va);

	int r;
	if((r = spawnl("hello", "mmaptestb", 0)) < 0) {
		panic("spawnl error:%e\n", r);
	}

	sleep(2);
	*(char*)(va + 4096 * 2-1) = '\0';
	cprintf("1: after b modified:[%.*s]\n", 4096*2,va);
	r = sys_munmap(va, 4096*2);
	cprintf("1: r=%d\n", r);
}
