/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "i8259.h"
#include "lib.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask = PIC_IRQ_MASK; /* IRQs 0-7  */
uint8_t slave_mask = PIC_IRQ_MASK;  /* IRQs 8-15 */


/* i8259_init()
 * DESCRIPTION: Initializes master and slave PICs on the 8259 chips by
 				manipulating the bits
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: establish master and slave relations
 */
/* Initialize the 8259 PIC */
void i8259_init(void) {
	/* mask interrupts in PIC */
	outb(PIC_IRQ_MASK, MASTER_8259_DATA_PORT);
	outb(PIC_IRQ_MASK, SLAVE_8259_DATA_PORT);

	/* set ICWs in order for the MASTER PIC */
	outb(ICW1, MASTER_8259_PORT);
	outb(ICW2_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW3_MASTER, MASTER_8259_DATA_PORT);
	outb(ICW4, MASTER_8259_DATA_PORT);

	//DEBUG: printf("master pic init success");

	//set ICWs in order for the SLAVE PIC
	outb(ICW1, SLAVE_8259_PORT);
	outb(ICW2_SLAVE, SLAVE_8259_DATA_PORT);
	outb(ICW3_SLAVE, SLAVE_8259_DATA_PORT);
	outb(ICW4, SLAVE_8259_DATA_PORT);
	
	//DEBUG: printf("slave pic init success");

}


/* enable_irq()
 * DESCRIPTION: Unmask IRQ # to give vector # of IRQ port
 * INPUTS: irq_num - # to unmask
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: distinguish IRQ # if on MASTER or SLAVE
 */
void enable_irq(uint32_t irq_num) {
	/* IRQ # is out of bounds (INVALID) */
	if(irq_num > NUM_IRQ){
		// DEBUG: printf("IRQ NUM is out of bounds! \n");
		return;
	}
	
	/* IRQ # correspondung to SLAVE */
	else if (irq_num > 7){ // slave IRQ is > 7
		irq_num -= 8;	/* get IRQ # on SLAVE */
		
		/* unmask IRQ#: SLAVE */
		slave_mask &= ~(0x1 << irq_num); // 0x1: base mask
		outb(slave_mask, SLAVE_8259_DATA_PORT);
		
		/* unmask IRQ#: MASTER */
		master_mask &= ~(0x1 << ICW3_SLAVE); // 0x1: base mask
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
	
	/* IRQ # correspondung to  MASTER */
    else{
    	master_mask &= ~(1 << irq_num);
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
}


/* disable_irq()
 * DESCRIPTION: Unmask IRQ # to give vector # of IRQ port
 * INPUTS: irq_num - # to mask
 * OUTPUTS: none
 * RETURN VALUE: none
 * SIDE EFFECTS: mask IRQ # for transport
 */
/* Disable (mask) the specified IRQ */
void disable_irq(uint32_t irq_num) {
	/* IRQ # is out of bounds (INVALID) */
	if(irq_num > NUM_IRQ){
		// DEBUG: printf("IRQ NUM is out of bounds! \n");
		return;
	}
	
	/* IRQ # correspondung to SLAVE */
	else if (irq_num > 7){
		irq_num -= 8;	/* get IRQ # on SLAVE */
		
		/* mask IRQ#: SLAVE */
		slave_mask |= (0x1 << irq_num); // 0x1: base mask
		outb(slave_mask, SLAVE_8259_DATA_PORT);
		
		/* mask IRQ#: MASTER */
		master_mask |= (0x1 << ICW3_SLAVE); // 0x1: base mask
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
	
	/* IRQ # correspondung to  MASTER */
    else{
    	master_mask |= (1 << irq_num);
		outb(master_mask, MASTER_8259_DATA_PORT);
	}
}


/*
 * send_eoi()
 *   DESCRIPTION: Sends EOI signal to IRQ port specified by IRQ #
 *   INPUTS: irq_num - port # EOI is sent to notify
 *   OUTPUTS: sends EOI signal
 *   RETURN VALUE: none
 *   SIDE EFFECTS: notify PICs that interrupt is done
 */
/* Send end-of-interrupt signal for the specified IRQ */
void send_eoi(uint32_t irq_num) {
	/* IRQ # is out of bounds (INVALID) */
	if(irq_num > NUM_IRQ){
		// DEBUG: printf("IRQ NUM is out of bounds! \n");
		return;
	}
	
	/* IRQ # correspondung to SLAVE */
	else if (irq_num > 7){ // slave IRQ is > 7
		irq_num -= 8;	/* get IRQ # on SLAVE */
		
		/* notify MASTER & SLAVE of EOI */
		outb((EOI | irq_num), SLAVE_8259_PORT);
		outb((EOI | 2), MASTER_8259_PORT); // 2: SLAVE IRQ PORT NUMBER
	}
	
	/* notify MASTER of EOI */
    else{
    	outb((EOI | irq_num), MASTER_8259_PORT);
	}
}



