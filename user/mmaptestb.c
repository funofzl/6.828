#include <inc/lib.h>

void
umain(int argc, char **argv) {
	int fd = 1;
	void* va = sys_mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	assert(va != NULL);
	cprintf("b: va:%08x\n", va);
	cprintf("b: old:[%s]\n", va);
	memset(va, 'y', 1024);
	*(char*)(va+1023) = '\0';
	cprintf("b: new:[%s]\n", va);
}
