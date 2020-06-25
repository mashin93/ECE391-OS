#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include "types.h"
#include "syscalls.h"

/* ======================== CONSTANTS DEFINITION ======================== */
#define PIT_IRQ			0	  	/* IRQ LINE to receive the PIT interrupts */
#define PIT_CMD_PORT	0x43  	/* command port for PIT */
#define PIT_INIT 		0x36  	/* command byte for PIT */
#define PIT_CHANNEL_0 	0x40  	/* IRQ PORT Number */

/* quick conversions */
#define _128MB 0x8000000
#define _8MB 0x800000
#define _4MB 0x400000
#define _8KB 0x2000

/* PIT Frequency: 1193180 / 20 to get HZ */	
#define CLK_FREQ	1193180	/* clock frequency to 20 second intervals */
#define SET_FREQ	50		/* desired frequency to set rate of PIT */

#define LSB_MASK		0xFF	/* get LSB in frequency */
#define MSB_MASK		8		/* MASK to shift for MSB */

#define MAX_TERMINALS	3		/* 0, 1, 2 indexs for 3 terminals */
#define TERMINAL_ACTIVE	1		/* terminal is active and avaliable */

/* ======================================================================= */

/* global that holds the current executing terminal index/id */
extern volatile uint8_t cur_active_terminal;


/* ======================== FUNCTION DECLARATION ======================== */
/* Initialize the frequency for PIT */
void PIT_init(void);

/* Schedules the PIT interrupt */
void PIT_scheduling(void);

/* carries out a contextswitch given a process */
void process_contextswitch(int next_process);

/* helper to fetch info and state of next scheduled processes */
int get_next_scheduled();


#endif
