#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/stdio.h>
#include <inc/string.h>

// LAB 6: Your driver code here
// TODO: self-code

volatile void* bar_va;

// Define TX desc array and packet buffer array.
struct e1000_tdh *tdh;
struct e1000_tdt *tdt;
struct e1000_tx_desc tx_desc_array[TXDESCS];
char tx_buffer_array[TXDESCS][TX_PKT_SIZE];


// Define RX desc array and packet buffer array.
struct e1000_rdh *rdh;
struct e1000_rdt *rdt;
struct e1000_rx_desc rx_desc_array[RXDESCS];
char rx_buffer_array[RXDESCS][RX_PKT_SIZE];

uint32_t E1000_MAC[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};

int pci_e1000_attach(struct pci_func* pcif) {
	// pci_func_enable会做一些协商，比如协商reg_base和reg_size，这就是mmio map的地址和大小
	pci_func_enable(pcif);
	// mmio_map_region
	bar_va = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
	uint32_t* status_reg = E1000REG(E1000_STATUS);
	cprintf("status: %08x\n", *status_reg);
	assert(*status_reg == 0x80080783);
	
	e1000_transmit_init();
	// Simple test transmit int kernel mode.
	// e1000_transmit("abc", 4);

	e1000_receive_init();
	return 0;
}



// Initialization of transmit.
void e1000_transmit_init() {
	// initialize all descs.
	int i;
	for(i=0; i < TXDESCS; i++) {
		memset(&tx_desc_array[i], 0, sizeof(struct e1000_tx_desc));
		tx_desc_array[i].addr = PADDR(&tx_buffer_array[i]);
		tx_desc_array[i].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
		tx_desc_array[i].status |= E1000_TXD_STAT_DD;
	}

	struct e1000_tdlen* tdlen = (struct e1000_tdlen*)E1000REG(E1000_TDLEN);
	//tdlen->len = TXDESCS; // should be set to the size of desc array
	*(uint32_t*)tdlen = TXDESCS * sizeof(struct e1000_tx_desc);

	// TDBAL regsiter
	uint32_t *tdbal = (uint32_t*)E1000REG(E1000_TDBAL);
	*tdbal = PADDR(tx_desc_array);

	// TDBAH register
	uint32_t *tdbah = (uint32_t*)E1000REG(E1000_TDBAH);
	*tdbah = 0;

	// TDH register, should be init to 0.
	tdh = (struct e1000_tdh *)E1000REG(E1000_TDH);
    tdh->tdh = 0;

	//TDT register, should be init 0
    tdt = (struct e1000_tdt *)E1000REG(E1000_TDT);
    tdt->tdt = 0;

	// Set TCTL register
	struct e1000_tctl *tctl = (struct e1000_tctl *)E1000REG(E1000_TCTL);
       tctl->en = 1;
       tctl->psp = 1;
       tctl->ct = 0x10;
       tctl->cold = 0x40;

	// Set TIPG register
	 struct e1000_tipg *tipg = (struct e1000_tipg *)E1000REG(E1000_TIPG);
       tipg->ipgt = 10;
       tipg->ipgr1 = 4;
       tipg->ipgr2 = 6;

}


// Exercise 6: Transmit packet
int e1000_transmit(void* data, size_t len) {
	// find the tail null desc in queue
	uint32_t current = tdt->tdt;
	// check the bit of DD
	if(!(tx_desc_array[current].status & E1000_TXD_STAT_DD)) {
		return -E_TRANSMIT_RETRY; // perhaps another process is transmiting a packet or the buffer is full, just retry.
	}
	tx_desc_array[current].length = len;
	tx_desc_array[current].status &= ~E1000_TXD_STAT_DD; // clear DD bit
	tx_desc_array[current].cmd |= (E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS);
	memcpy(tx_buffer_array[current], data, len);
	uint32_t next = (current + 1) % TXDESCS;
	tdt->tdt = next;
	return 0;
}

static void
get_ra_address(uint32_t mac[], uint32_t *ral, uint32_t *rah)
{
       uint32_t low = 0, high = 0;
       int i;

       for (i = 0; i < 4; i++) {
               low |= mac[i] << (8 * i);
       }

       for (i = 4; i < 6; i++) {
               high |= mac[i] << (8 * i); // 这里为什么不是8*(i-4)呢 因为int移动的位数超过32之后会对32取余的
       }

       *ral = low;
       *rah = high | E1000_RAH_AV;
}

// Exercise 8: Receive Initialization
void e1000_receive_init() {
	// 1. allocate memory for desc array and set RDBAL/RDBAH
	// 2. set RDLEN()
	// 3. Set RDBAL/RDBAH
	// 4. Set RDH(head) and RDT(tail)
	// 5. Set RCTL
	// 6. Set RAL/RAH
	int i;
	for(i=0;i<RXDESCS;i++) {
		memset(&rx_desc_array[i], 0, sizeof(struct e1000_rx_desc));
		rx_desc_array[i].addr = PADDR(rx_buffer_array[i]);
	}

	// RDLEN = RDDESCS * sizeof(desc)
	uint32_t *rdlen = (uint32_t*)E1000REG(E1000_RDLEN);
	*rdlen = RXDESCS * sizeof(struct e1000_rx_desc);

	// set rdbal(addr of desc array) and rdbah(0)
	uint32_t* rdbal = (uint32_t*)E1000REG(E1000_RDBAL);
	uint32_t* rdbah = (uint32_t*)E1000REG(E1000_RDBAH);
	*rdbal = PADDR(&rx_desc_array[0]);
	*rdbah = 0; // why zero?
	
	rdh = (struct e1000_rdh*)E1000REG(E1000_RDH);
	rdh->rdh = 0;
	rdt = (struct e1000_rdt*)E1000REG(E1000_RDT);
	rdt->rdt = RXDESCS - 1; // 注意和transmit的区别 那个是head ~ tail中间的部分是有数据的，这个是head ~ tail的部分是空闲的

	// set RCTL
	uint32_t* rctl = (uint32_t*)E1000REG(E1000_RCTL);
	*rctl = E1000_RCTL_EN | E1000_RCTL_BAM | E1000_RCTL_SECRC;

	uint32_t* ra = (uint32_t*)E1000REG(E1000_RA);
	uint32_t ral, rah;
	get_ra_address(E1000_MAC, &ral, &rah);
	ra[0] = ral;
	ra[1] = rah;
	//cprintf("%08x %08x\n", ral, rah);
}

int e1000_receive(void* addr, size_t *len) {
	int32_t next = (rdt->rdt + 1) % RXDESCS;
    if(!(rx_desc_array[next].status & E1000_RXD_STAT_DD)) {	//simply tell client to retry
		return -E_RECEIVE_RETRY;
    }
    if(rx_desc_array[next].errors) {
        cprintf("receive errors\n");
        return -E_RECEIVE_RETRY;
    }
    *len = rx_desc_array[next].length;
    memcpy(addr, &rx_buffer_array[next], *len);

	rdt->rdt = next;
    return 0;
}

