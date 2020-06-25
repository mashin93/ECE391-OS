#include "tests.h"
#include "x86_desc.h"
#include "lib.h"

/* include files for tests */
#include "i8259.h"
#include "paging.h"
#include "keyboard.h"
#include "rtc.h"
#include "filesys.h"
#include "terminal.h"
#include "syscalls.h"

#define PASS 1
#define FAIL 0


/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* ==================================== CHECKPOINT 1 TESTS =============================START== */
#define NON_GENERGIC_IDT	19		/* 0-18 is non generic interrupts: total of 19 entries */
#define GENERIC_ENTRY		25		/* we want to call the 25th interrup that is generic */
/* IDT Test - Example -- IDT
 * DESCRIPTION: Asserts that first 18 IDT entries are not NULL
 * Inputs: none
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test() {
	TEST_HEADER;

	int i;
	int result = PASS;
	/* check the non generic interrupts */
	for (i = 0; i < NON_GENERGIC_IDT; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			printf("failed at IDT %d \n", i);
			assertion_failure();
			result = FAIL;
		}
	}

	/* check for the generic blue screen ones */
	if ((idt[GENERIC_ENTRY].offset_15_00 == NULL) && 
		(idt[GENERIC_ENTRY].offset_31_16 == NULL)){
			printf("failed at IDT 25");
			assertion_failure();
			result = FAIL;
	}

	return result;
}


/*
 * irq__masking_test -- PIC
 *   DESCRIPTION: enable and disable at different IRQ numbers for 
 *				  out of bounds, SLAVE PIC, and MASTER PIC
 *   INPUTS: none
 *   OUTPUTS: PASS/FAIL
 *   SIDE EFFECTS: unmasks then masks IRQ #s
 *   COVERAGE: enable_irq, disable_irq
 *   FILES: i8259.c/h
 */
#define NUM_IRQS		16		/* number if IRQ vector numbers on master and slave */
#define TEST_IRQ		8		/* we want to enable/disable IRQ8 */

int irq_masking_test() {
	TEST_HEADER;
	
	/* disable actual interrups during the tests */
	cli();

	/* check all IRQ # masking */
	int i = 0;
	for(i = 0; i < NUM_IRQS; i++) {
		enable_irq(i);
		disable_irq(i);
	}

	/* test IRQ# on MASTER : multiple calls */
	enable_irq(1); enable_irq(1); 
	disable_irq(1); disable_irq(1);

	/* test IRQ# on SLAVE : disable before enable */
	disable_irq(TEST_IRQ);
	enable_irq(TEST_IRQ);
	disable_irq(TEST_IRQ);

	/* test IRQ# Out Of Bounds */
	enable_irq(NUM_IRQS);
	disable_irq(NUM_IRQS);

	sti();
	return PASS;
}

#define RTC_IRQ_NUM	  	8
#define STALL_TIME 		50000000
/*
 * rtc_test -- RTC
 *   DESCRIPTION: check functionalities of rtc interrupt handling
 *   INPUTS: none
 *   OUTPUTS: write errors to screen
 *   SIDE EFFECTS: test enable/disable of rtc
 *   COVERAGE: rtc initialization and how it handles interrupts
 *   FILES: rtc.h/c, interrupthandler.S
 */
int rtc_test() {
	TEST_HEADER;

	/* initialize rtc */
	init_rtc();

	/* unmask RTC interrupt */
	enable_irq(RTC_IRQ_NUM);
	
	/* stall to read */
	volatile unsigned long arb = 0;
	while(arb < STALL_TIME) arb++;

	/* turn on interrupts */
	sti();

	/* stall interrupts */
	arb = 0;
	while (arb < STALL_TIME) arb++;

	/* mask RTC interrupt */
	disable_irq(RTC_IRQ_NUM);

	/* test specific functions in files */
	send_eoi(RTC_IRQ_NUM);
	//test_interrups();

	return PASS;
}



/*
 * exception_test
 *   DESCRIPTION: check for initialization of IDT Exceptions
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: should shout KERNEL PANIC
 *   COVERAGE: exception_handler and the IDT
 *   FILES: exception_handler.h/c ,x86_desc.h/S, idt.h/c
 */
#define TEST_DIV	10
int exception_test(){
	TEST_HEADER;

	int divider = 0;

	/* call Interrupt 0: DIVIDE BY ZERO */
	int condition;
	condition = TEST_DIV / divider;
	if(condition == 0) {
	assertion_failure();
	}

	/* Throw reserved , generic exception to screen */
	//assertion_failure();
	return FAIL;
}


/*
 * system_trap_test
 *   DESCRIPTION: check for setup of system trap
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: output message to screen
 *   COVERAGE: system trap set ups
 *   FILES: interrupt_handler.h/c, interrupthandler.S, idt.h/c
 */
int trap_test(){
	TEST_HEADER;
  	asm volatile("int $0x80"); 		/*testing trap number = 0x80*/
	return PASS;
}


#define KERNEL_LOCATION	0x400000		/* where the kernel page starts */
#define VIDMEM_LOCATION	0xB8000			/* where the video memory starts */
#define NUM_ENTRIES		1024			/* number of entries in paging 4kb */
/* Initalize Paging Test -- PAGING
 * DESCRIPTION: Check pointers of directories & tables to see if attribute bits set correctly
 * 				If the control register bits are enabled and set correctly
 * Input: none
 * Output: PASS/FAIL
 * Side Effects: none
 * Coverage: page_directory[0] and [1] != NULL, page_table[vid_mem] != NULL
 * Files: paging.h/c
 */
int paging_init_test()  {
	TEST_HEADER;
	printf(" ==== PAGING TEST 1 ==== \n");
	int* page_directory;
	int *ptr = page_directory;
	/* check for null first_page_table ptr */
	if( ptr[0] == NULL)  {
		return FAIL;
	} else {
		printf(" -> First_page_table ptr initialized: CORRECTLY :) \n");
	}

	/* check for null kernel page ptr */
	if( ptr[1] == NULL)  {
		return FAIL;
	} else {
		printf(" -> Kernel page ptr initialized: CORRECTLY :) \n");
	}

	/* check for null vid mem page ptr */
	if( ptr[VIDMEM_LOCATION] == NULL)  {
		return FAIL;
	} else {
		printf(" -> Video Mem page ptr initialized: CORRECTLY :) \n\n");
	}
	
	return PASS;
}



/* Write/Read Paging Test -- PAGING
 * DESCRIPTION: Write to and read from a valid address at 0 in 1st page_table
 				Write to and read from a valid address in the Kernel page
 *				Write to and read from a valid address at Video Mem address
 * Inputs: none
 * Outputs: PASS/FAIL
 * Side Effects: none
 * Coverage: declaration and initialization of kernel page in memory
 * Files: paging.h/c
 */
int rw_paging_test(){
	TEST_HEADER;
	printf(" ==== PAGING TEST 1 ==== \n");
	
	/* R/W test at kernel */
 	unsigned int * arb;
    unsigned int arb_in;
    arb = (unsigned int *)KERNEL_LOCATION;
    arb_in = *arb;


	/* R/W test at vid mem */
	int arb2,i;
    int * vm = (int *)VIDMEM_LOCATION;
    for(i = 0; i < NUM_ENTRIES ; i++){
        arb2 = vm[i];
    }

	return PASS;
}


#define NULL_ADDR		0x0				/* NULL ADDRESS TO CAUSE ERRORS */
#define KERNEL_ADDR		0xFFFFFF0		/* ARBITRARY KERNEL ADDRESS TO CAUSE ERRORS */
/*
 * paging_test_failure()
 *   DESCRIPTION: Tries to dereference invalid pages. All should result in page
 *                  fault. If test returns, then paging is not set up.
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: Causes a page fault
 *   COVERAGE: basic paging support
 *   FILES: paging.h/c, paging_asm.S
 */
int paging_deref_test(){
	TEST_HEADER;
	printf(" ==== PAGING TEST 2 ==== \n");

	/* deref a NULL page */
	int *a = (int*) NULL_ADDR;
	a++;

	/* deref KERNEL page */
	a = (int*) KERNEL_ADDR;
	a++;
	
	/* deref VID MEM page */
	a = (int*) VIDMEM_LOCATION;
	a++;

	return FAIL;
}

/* =============================================================================END== */


/* =============================== CHECKPOINT 2 TESTS ========================START== */
/*
 * terminal_syscall_open_close()
 *   DESCRIPTION: see if only 0 is returned and nothing else is altered
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: no major side effects
 *   COVERAGE: check functionality of open and close
 *   FILES: terminal.h/c
 */
int
terminal_syscall_open_close(){
 	TEST_HEADER;
	int result;

	/* test terminal_open(void) returns 0 */
	result = terminal_open(0);
	if(result != 0) {
		assertion_failure(); 
		return FAIL;
	}

	/* test terminal_close(void) returns 0 */
	result = terminal_close(0);
	if(result != 0) {
		assertion_failure(); 
		return FAIL;
	}
	
	/* both open and close works correctly */
	return PASS;
}


/*
 * test_terminal_read()
 *   DESCRIPTION: checks functionality of terminal read:
 				  -- receiving null ptr
 				  -- change max number to >, =, <
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: no major side effects, interrupts not blocked
 *   COVERAGE: check functionality of read
 *   FILES: terminal.h/c
 */
int
test_terminal_read() {
	TEST_HEADER;
	int result;
	char buffer[300] = {'\n'};

	/* CASE1: test when NULL ptr passed in, with nothing to read */
	printf("\nttr_CASE1: ");
	result = terminal_read(0,NULL, 0);
	if(result != -1) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE2: test when NULL ptr passed in, with something to read */
	printf("\nttr_CASE2: ");
	result = terminal_read(0,NULL, 5);
	if(result != -1) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE3: test when buffer passed in, with nothing to read */
	printf("\nttr_CASE3: ");
	result = terminal_read(0,buffer, 0);
	if(result != 0) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE4: test when buffer passed in, with 5 chars to be read */
	printf("\nttr_CASE4: ");
	result = terminal_read(0,buffer, 5);
	if(result != 5) {
		assertion_failure(); 
		return FAIL;
	}
	terminal_write(0,buffer, 5);

	/* CASE5: test when buffer passed in, with 200 > 128 to be read */
	printf("\nttr_CASE5: \n");
	result = terminal_read(0,buffer, 200);
	if(result != 128) {
		assertion_failure(); 
		return FAIL;
	}
	terminal_write(0,buffer, 128);

	return PASS;
}



/*
 * test_terminal_write()
 *   DESCRIPTION: checks functionality of terminal write:
 				  -- receiving null ptr
 				  -- change max number to >, =, <
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: no major side effects, interrupts not blocked
 *   COVERAGE: check functionality of write
 *   FILES: terminal.h/c
 */
int
test_terminal_write() {
	TEST_HEADER;
	int result;

	/* CASE1: test when NULL ptr passed in, with nothing to write */
	printf("\nterm_write_CASE1: print 0 chars --> ");
	result = terminal_write(0,NULL, 0);
	if(result != -1) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE2: test when NULL ptr passed in, with something to write */
	printf("\nterm_write_CASE2: print 0 chars --> ");
	result = terminal_write(0,NULL, 5);
	if(result != -1) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE3: test when buffer passed in, with nothing to write */
	printf("\nterm_write_CASE3: print 0 chars --> ");
	result = terminal_write(0,"empty", 0);
	if(result != 0) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE4: test when buffer passed in, with 5 chars to be write */
	printf("\nterm_write_CASE4: print 4 chars --> ");
	result = terminal_write(0,"fo ur", 4);
	if(result != 4) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE5: test when buffer passed in, with 200 > 128 to be write */
	printf("\nterm_write_CASE5: print 5 chars --> ");
	result = terminal_write(0,"hello", 5);
	if(result != 5) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE5: test when buffer passed in, with 200 > 128 to be write */
	printf("\nterm_write_CASE5: print 5 chars --> ");
	result = terminal_write(0,"hello", 6);
	if(result != 5) {
		assertion_failure(); 
		return FAIL;
	}

	/* CASE6: test when buffer passed in, with \0 to be write */
	printf("\nterm_write_CASE5: print 3 chars --> ");
	result = terminal_write(0,"thr\0ee", 8);
	if(result != 3) {
		assertion_failure(); 
		return FAIL;
	}
	printf("\n");

	return PASS;
}



/*
 * rtc_writefreq_test()
 *   DESCRIPTION: Tries to test if frequencies power of 2 will return 0, and if numbits is not 4
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: rtc_write()
 *   FILES: rtc.h/rtc.c
 */
int rtc_writefreq_test(){
	TEST_HEADER;

	/* test if frequency is power of 2, first two are not */
	int freq = 17;
	if (rtc_write(0,&freq, 4) == 0){
	printf("freq: 17 ");
	return FAIL;
	}
	freq = -15;
	if (rtc_write(0,&freq, 4) == 0) {
	printf("freq = -15 ");
	return FAIL;
	}

	/* test if frequency is power of 2, last two are */
	freq = 8;
	if (rtc_write(0,&freq, 4) == -1) {
	printf("freq = 8 ");
	return FAIL;
	}
	freq = 16;
	if (rtc_write(0,&freq, 4) ==  -1) {
	printf("freq = 16 ");
	return FAIL;
	}

	/* test if numbits is not 4 */
	if (rtc_write(0,&freq, 6) ==  0) {
	printf("testing wrong frequency ");
	return FAIL;
	}
	return PASS;
}



/*
 * rtc_write_test()
 *   DESCRIPTION: Tries to test if frequencies power of 2 will return 0, and if numbits is not 4
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: rtc_write()
 *   FILES: rtc.h/rtc.c
 */
int rtc_write_test(){
	TEST_HEADER;


	int freq = 4; //test frequency

	int result;
	int i = 0;

	printf("\ninput frequency is %d\n", freq);

	result = rtc_write(0,&freq, 4); // freq configuration


//	printf("\nfrequency is %d\n", result);

	while (i < 15){ 
	if (!rtc_read(0,&freq, 4)){ // when there is an interrupt
		printf("1");
		i++;
		}
	}

	printf("\nChanging frequency from 1 to 50:");
	freq = 50;
//	printf("\nout of loop\n");
	result = rtc_write(0,&freq, 4); // freq configuration
	int j = 0;
	while (j < 100){ 
	if (!rtc_read(0,&freq, 4)){ // when there is an interrupt
		printf("#");
		j++;
		}
	}

	printf("\nChanging frequency from 50 to 1000:");
	freq = 1000;
//	printf("\nout of loop\n");
	result = rtc_write(0,&freq, 4); // freq configuration
	int k = 0;
	while (k < 1000){ 
	if (!rtc_read(0,&freq, 4)){ // when there is an interrupt
		printf("@");
		k++;
		}
	}
	

	return PASS;
}



/*
 *	 rtc_open_test()
 *   DESCRIPTION: tries to see if rtc drive will open succesfully 
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: rtc_open()
 *   FILES: rtc.h/rtc.c
 */
int rtc_open_test(){
	TEST_HEADER;
	/* if opens then should return 0 */
	if (rtc_open(0) != 0) {
		assertion_failure();
		return FAIL;
	}
	/* do it again just in case */
	if (rtc_open(0) != 0) {
		assertion_failure();
		return FAIL;
	}
	return PASS;
}




/*
 *	 rtc_close_test()
 *   DESCRIPTION: tries to see if rtc drive will close succesfully 
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: rtc_close()
 *   FILES: rtc.h/rtc.c
 */
int rtc_close_test() {
	TEST_HEADER;
	/* if closes then should return 0 */
	if (rtc_close(0) != 0) {
		assertion_failure();
		return FAIL;
	}
	/* try once more */
	if (rtc_close(0) != 0) {
		assertion_failure();
		return FAIL;
	}

	return PASS;
}


/* =======================================================================================END== */


/* =============================== CHECKPOINT 3 TESTS ==================================START== */

/*
 *	 PCB_init_test()
 *   DESCRIPTION: try to see if PCB only takes valid pids and initialize them correctly 
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if pcb is correctly initialized
 *   FILES: system_call.c
 */
int PCB_init_test()  {
	TEST_HEADER;

	// /* giving PCB invalid pids to initialize */
	// /* check for negative numbers */
	// if(init_pcb(-1) != (pcb_t*)-1) {
	// 	return FAIL;
	// }
	//  check for pids that are greater than 
	//    what is existing: 2 for this checkpoint 
	// if(init_pcb(10) != (pcb_t*)-1) {
	// 	return FAIL;
	// }

	// // /* PCB catches invalid inputs */
	 return PASS;
}


/*
 *	 open_sys_test()
 *   DESCRIPTION: test if syscall only opens files with valid names
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if open sys open legit files only
 *   FILES: system_call.c
 */
int open_sys_test()  {
	TEST_HEADER;

	/* create a process by generating pid and initialize it */
	// current_pid = 0;
	// init_pcb(0);

	/* check if NULL file name will be catched */
	if(open(NULL) != -1) {
		return FAIL;
	}

	/* try to open a file name that DNE (can't be found) */
	if(open((uint8_t*)"FILE_DNE") != -1) {
		return FAIL;
	}

	//maybe add in a test that can test the opening of different types of files

	return PASS;
}


/*
 *	 close_sys_test()
 *   DESCRIPTION: test if syscall closes files in correct order
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if close can only close files that are on going, 
 			   also doesn't try to close the 0th process
 *   FILES: system_call.c
 */
int close_sys_test()  {
	TEST_HEADER;

	/* create a process by generating pid and initialize it */
	// current_pid = 0;
	// init_pcb(0);

	/* check if file descriptor index within boundaries */
	/* when there is no file */
	if(close(0) != -1)
		return FAIL;
	/* greater than current pid */
	if(close(1) != -1)
		return FAIL;
	/* greater than fd max */
	if(close(12) != -1)
		return FAIL;
	/* less than 0 */
	if(close(-1) != -1)
		return FAIL;

	// should we call a function here and test again?

	return PASS;
}



/*
 *	 read_sys_test()
 *   DESCRIPTION: test if syscall reads files correctly
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if read catches all the errors like invalid paramaters
 *   FILES: system_call.c
 */

#define TEST_BUF_LENGTH		38
int read_sys_test()  {
	TEST_HEADER;

	/* generate buffer as from a file */
	uint8_t test_buf[TEST_BUF_LENGTH]="BETTER READ THIS OR ITS BAD NEWS BARES";
	
	/* create a process by generating pid and initialize it */
	// current_pid = 0;
	// init_pcb(0);

	/*  read(int32_t fd, void* buf, int32_t nbytes)  */
	/* invalid buffer */
	if(read(0, NULL, TEST_BUF_LENGTH) != -1)
		return FAIL;
	/* invalid fd < 0  */
	if(read(-1, test_buf, TEST_BUF_LENGTH) != -1)
		return FAIL;
	/* invalid num bytes to copy < 0 */
	if(read(1, test_buf, -23) != -1)
		return FAIL;
	/* fd is greater than fd max */
	if(read(100, test_buf, TEST_BUF_LENGTH) != -1)
		return FAIL;

// how to test if there is no such file

	return PASS;
}


/*
 *	 write_sys_test()
 *   DESCRIPTION: test if syscall writes files correctly
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if write catches all the errors like invalid paramaters
 *   FILES: system_call.c
 */
int write_sys_test()  {
	TEST_HEADER;

	/* generate buffer as from a file */
	uint8_t test_buf[TEST_BUF_LENGTH];

	/* create a process by generating pid and initialize it */
	// current_pid = 0;
	// init_PCB(0);

	if(write(1, NULL, TEST_BUF_LENGTH) != -1)
		return FAIL;
	if(write(-1, test_buf, TEST_BUF_LENGTH) != -1)
		return FAIL;
	if(write(0, test_buf, TEST_BUF_LENGTH) != -1)
		return FAIL;
	if(write(100, test_buf, TEST_BUF_LENGTH) != -1)
		return FAIL;

	return PASS;
}


/*
 *	 execute_test()
 *   DESCRIPTION: test if syscall execute files does work and calls the righ things correctly
 *   INPUTS: none
 *   OUTPUTS: none
 *   SIDE EFFECTS: none
 *   COVERAGE: if execute catches all the errors like invalid paramaters
 *   FILES: system_call.c
 */
int execute_test()  {
	TEST_HEADER;

	if(execute(NULL) != -1)
		return FAIL;
	/* try to exectute a fake files */
	if(execute((uint8_t*)"FAKE_FILE") != -1)
		return FAIL;

	//  execute a real file to see if it goes through with no errors 
	// if(execute((uint8_t*)"frame1.txt") != -1)
	// 	return FAIL;

	// needs more concrete tests but not finished with it yet

	return PASS;
}

/* =======================================================================================END== */


/* Test suite entry point */
void launch_tests(){
	//TEST_OUTPUT("idt_test", idt_test());
	
	/* so we can see test outputs*/
	volatile unsigned long arb = 0;
	while(arb < STALL_TIME) arb++; 

	/* so the screen does not overwrite */
	clear();
	printf(" ========== STARTING TESTS ==========\n");

	/* ============================================== launch CHECKPOINT 3 TESTS here */
	// TEST_OUTPUT("open_sys_test()", open_sys_test());
	// TEST_OUTPUT("close_sys_test()", close_sys_test());
	// TEST_OUTPUT("read_sys_test()", read_sys_test());
	// TEST_OUTPUT("write_sys_test()", write_sys_test());

	// /* tests for execute */
	// TEST_OUTPUT("execute_test()", execute_test());
	/* ============================================================== END CKPT3 ==== */

	/* ============================================== launch CHECKPOINT 2 TESTS here */
	/* terminal tests */
	//TEST_OUTPUT("terminal_syscall_open_close()", terminal_syscall_open_close()); 
	//TEST_OUTPUT("test_terminal_read()", test_terminal_read()); 
	//TEST_OUTPUT("test_terminal_write()", test_terminal_write()); 


	/* rtc tests */
	//TEST_OUTPUT("rtc_writefreq_chp_2", rtc_writefreq_test());
	//TEST_OUTPUT("rtc_write_chp_2", rtc_write_test());
	//TEST_OUTPUT("rtc_open_test", rtc_open_test());
	//TEST_OUTPUT("rtc_close_test", rtc_close_test());


	/* file system tests */
	//TEST_OUTPUT("filetests", filetests());
	//TEST_OUTPUT("printallfiles_test", printallfiles_test());
	//TEST_OUTPUT("morefiletests", morefiletests());
	/* ============================================================== END CKPT2 ==== */

	/* ============================================== launch CHECKPOINT 1 TESTS here */
	//TEST_OUTPUT("idt_test", idt_test());
	//TEST_OUTPUT("irq_masking_test", irq_masking_test());
	//TEST_OUTPUT("rtc_test", rtc_test());
	//TEST_OUTPUT("trap_test", trap_test());
	//TEST_OUTPUT("rw_paging_test", rw_paging_test());
	//TEST_OUTPUT("paging_init_test", paging_init_test()); 

	/* tests that will throw exceptions */
	//TEST_OUTPUT("exception_test", exception_test());
	//TEST_OUTPUT("paging_deref_test", paging_deref_test());
	/* ============================================================== END CKPT1 ==== */
}


/* =============================== CHECKPOINT 4 TESTS ========================START== */
/* =============================================================================END== */
/* =============================== CHECKPOINT 5 TESTS ========================START== */
/* =============================================================================END== */
