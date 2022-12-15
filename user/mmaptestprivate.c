#include <inc/lib.h>


void
umain(int argc, char **argv) {
	int fd = 1;
	void* va = sys_mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	cprintf("1: va:%08x\n", va);
	assert(va != NULL);
	cprintf("1: old:[%s]\n", va);
	memset(va, 'x', 1024);
	*(char*)(va+1023) = '\0';
	cprintf("1: new:[%s]\n", va);

	int r;
	if((r = spawnl("hello", "mmaptestb", 0)) < 0) {
		panic("spawnl error:%e\n", r);
	}

	sleep(3);
	cprintf("1: after b modified:[%s]\n", va);
}
