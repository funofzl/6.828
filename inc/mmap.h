#include <inc/memlayout.h>


#define PROT_READ	0x1
#define PROT_WRITE	0x2
#define PROT_EXEC	0x4
#define PROT_NONE	0x8
#define PROT_MASK	0xf

#define MAP_SHARED  0x10
#define MAP_PRIVATE	0x20
#define FLAG_MASK	~PROTMASK



struct MmapRegion{
	int fd;
	off_t offset;
	off_t end;		// perhaps len < 4096, but we can also operate the data whose size is up to PGSIZE.
	int flags;		// shared or private (later anosy..)
	uint32_t va;	// the mmap va address 
	int next;		// next map_region of this process.
};
