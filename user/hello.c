#include <inc/lib.h>

void
umain(int argc, char **argv) {
	int fd = 1;
	//void* va = sys_mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	void* va = sys_mmap(NULL, 4096*2, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4096);
	assert(va != NULL);
	cprintf("b: va:%08x\n", va);
	cprintf("b: old:[%s]\n", va);
	memset(va, 'y', 4096*2);
	*(char*)(va+4096*2-1) = '\0';
	cprintf("b: new:[%s]\n", va);
	int r = sys_munmap(va, 4096*2);
	cprintf("b: r=%d\n", r);
	cprintf("b: new:[%s]\n", va);
}
