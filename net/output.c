#include "ns.h"
#include "inc/lib.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid)
{
	binaryname = "ns_output";

	// LAB 6: Your code here:
	// 	- read a packet from the network server
	//	- send the packet to the device driver
	// TODO: self-code
	uint32_t whom;
	int perm;
	int32_t req;

	while(1) {
		req = ipc_recv((envid_t*)&whom, &nsipcbuf, &perm);
		if(req != NSREQ_OUTPUT) {
			cprintf("not a nsreq outpyt\n");
			continue;
		}
		struct jif_pkt* pkt = &(nsipcbuf.pkt);
		//cprintf("%08x %d\n", pkt->jp_data, pkt->jp_len);
		while(sys_pkt_send((void*)pkt->jp_data, (size_t)pkt->jp_len) < 0) {
			sys_yield();
		}
	}
}
