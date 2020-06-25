#include "scheduler.h"
#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "i8259.h"
#include "syscalls.h"
#include "paging.h"
#include "terminal.h"

/* ====================== GLOBAL VARIABLE DECLARATIONS ======================= */
/* holds index to current terminal that is executing the process */
volatile uint8_t cur_active_terminal = 0; 
/* holds index to next terminal that is scheduled to execute process */   
         uint8_t next_sched_terminal = 0;       
/* =========================================================================== */

/* PIT_init
 *   DESCRIPTION: Initialize PIT to allow interrupts for scheduling.
 *                Consulted wiki.osdev.org (Programmable_Interval_Timer -- PIT)
 *   INPUT: none
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: IRQ 0 is allowed to get interrupts generated from the PIT
 */
void PIT_init() {
    /* calculate frequency */
    uint16_t frequency = CLK_FREQ / SET_FREQ;

    /* Set interrupt intervals to be 20 seconds for scheduling */
    outb(PIT_INIT, PIT_CMD_PORT);
    outb(frequency & LSB_MASK, PIT_CHANNEL_0);
    outb(frequency >> MSB_MASK, PIT_CHANNEL_0);
    
    /* Enable IRQ 0 to receive PIT interrupts */
    enable_irq(PIT_IRQ);
    
    return;
}




/* PIT_scheduling
 *   DESCRIPTION: locks and carryout context switch after receiving PIT interrupts
 *   INPUT: none
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: calls contextswitch
 */
void PIT_scheduling() {
    
    /* send EOI to PIT */
    send_eoi(PIT_IRQ); 
    
    cli(); /* disable interrupts */
    /* check to see if terminal 2 or 3 is running, if yeah then find next scheduled process */
    if (terminal[1].active == 1 || terminal[2].active == 1) {
            process_contextswitch(get_next_scheduled());
    }
    sti(); /* enable interrupts again */
    
    return;
}

/* process_contextswitch
 *   DESCRIPTION: Switch from one process to another, given the next scheduled process number
 *   INPUT: process_number -- the id of the process that we need to switch to
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: context switch with saving/swaping ESP/EBP
 */
void process_contextswitch(int next_process) {
    /* create a new page in V_ADDR 8MB --> P_ADDR 8 MB */
    page_remap(_128MB, _8MB + next_process * _4MB);


    /* Video memory remapping: 136 MB */
    uint8_t *screen_start;
    vidmap(&screen_start);


    /* saving and restoring PCB and states */
    /* Get old PCB: switch FROM */
    pcb_t * old_pcb = get_pcb_ptr_process(terminal[cur_active_terminal].apn);
    /* update global variable for current active terminal */
    cur_active_terminal = next_sched_terminal;
    /* Get new PCB: switch TO */
    pcb_t * next_pcb = get_pcb_ptr_process(next_process);
    

    /* Fetch correct terminal with new PCB */
    term_t * terminal = next_pcb->term;

    /* Remap the video memory to buffer if the current active terminal is not displayed to user currently */
    if (terminal->id != cur_term) {
        vidmem_remap((uint32_t)screen_start, (uint32_t)terminal->video_mem);
    }

    /* CONTEXT SWITCH: Save SS0 and ESP0 */
    tss.esp0 = _8MB - _8KB * (next_process) - 4;
    tss.ss0 = KERNEL_DS;

    /* CONTEXT SWITCH: Swap ESP/EBP */
    asm volatile(
        "movl %%esp, %%eax;"
        "movl %%ebp, %%ebx;"
        :"=a"(old_pcb->esp), "=b"(old_pcb->ebp)    /* OUTPUT: pointers of old pcb */
        :                                          /* INPUT: none */
        );
                 
    asm volatile(
        "movl %%eax, %%esp;"
        "movl %%ebx, %%ebp;"
        :                                          /* OUTPUT: none, */
        :"a"(next_pcb->esp), "b"(next_pcb->ebp)    /* INPUT: pointers of new pcb */
        );
    
    return;
}



/* get_next_scheduled
 *   DESCRIPTION: obtains the process number for the process to be executed next according to scheduling
 *   INPUT: none
 *   OUTPUT: the process number fo the next scheduled process
 *   RETURN VALUE: none
 *   SIDE EFFECT: will change the next_scheduled_terminal used in function that will be called in Context Switch
 */
int get_next_scheduled(){
    /* Loop Through active terminal: 0 ~ 2 */
    unsigned int i;
	
    /* update the next_scheduled terminal */
    next_sched_terminal = cur_active_terminal;

    /* loop through the terminals to find next schedule terminal (any other active terminal) */
    for (i = 0; i < MAX_TERMINALS - 1; i++) {
        next_sched_terminal = (next_sched_terminal+1) % MAX_TERMINALS;
        if (terminal[next_sched_terminal].active == TERMINAL_ACTIVE)
            break;
    }

    /* give most recent process number for next process */
    return (int)(terminal[next_sched_terminal].apn);
    
}

