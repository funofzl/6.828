#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;

// allocate a region memory as the buffer of nsipcbuf which should send to the network system env.
//char buffers[10][PGSIZE];

static struct jif_pkt *pkt = (struct jif_pkt*)REQVA;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	// LAB 6: Your code here:
	// 	- read a packet from the device driver
	//	- send it to the network server
	// Hint: When you IPC a page to the network server, it will be
	// reading from it for a while, so don't immediately receive
	// another packet in to the same physical page.
	// TODO: self-code
	// send ipe and then set NO_RUNNABLE and yield.
	// 必须alloc pahe 才可以
	int r;

	size_t len;
	while(true) {
		// 每次都要重新申请一块物理内存映射到特定的虚拟地址REQVA处
		// ipc_send会将这块物理地址映射到receiver的虚拟地址空间 这个时候物理page的ref_count=2
		// 然后再unmap 那么这块物理地址就只属于receiver的了 下一次继续分配新的物理页
		if ((r = sys_page_alloc(0, pkt, PTE_P|PTE_U|PTE_W)) < 0) {
			panic("sys_page_alloc: %e", r);
		}
		while(sys_pkt_recv((void*)&pkt->jp_data, &len) < 0) {
			sys_yield();
		}
		pkt->jp_len = len;
		ipc_send(ns_envid, NSREQ_INPUT, (void*)pkt, PTE_P | PTE_U);
		r = sys_page_unmap(0, (void*)pkt);
		if(r < 0) {
			panic("in input.c, sys_page_unmap error: %e\n", r);
		}
		//sys_yield();
	}
}
