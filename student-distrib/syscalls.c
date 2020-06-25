#include "scheduler.h"

/* ====================== DECLARE GLOBAL VARIABLES ====================== */
/* Process ID Array to start a new process - only can have 6 at a time */
uint8_t pid_array [NUM_MAX_PROCESSES] = {0, 0, 0, 0, 0, 0};
/* keeps track of which current process it is running */
uint32_t curr_process = 0;
/* ====================================================================== */



/* ======= fops table ptrs for STDIN/STDOUT ======== */
fops_t fops_STDIN = {  terminal_read, 
						has_error, 
						terminal_open, 
						terminal_close };

fops_t fops_STDOUT = { has_error, 
						terminal_write, 
						terminal_open, 
						terminal_close };


/* ========= fops table ptrs for FILETYPES ========== */
fops_t fops_rtc = {	rtc_read, 
					rtc_write, 
					rtc_open, 
					rtc_close };

fops_t fops_dir = {	directory_read, 
					directory_write, 
					directory_open, 
					directory_close };

fops_t fops_file = {file_read, 
					file_write, 
					file_open, 
					file_close };

/* ========= fops table ptrs for ERRORS ========== */
fops_t fops_error = {has_error, 
					has_error, 
					has_error, 
					has_error };




/* open
 * DESCRIPTION: system call for open, attempts to open a file with its filename
 * INPUTS: filename
 * OUTPUTS: populates file array
 * RETURN VALUE: 0 on success, -1 on fail
 * SIDE EFFECTS: open file
 */
int32_t sys_open(const uint8_t* filename) {
	dentry_t dir_entry;
	/* check if file with name exist in system */
	if (read_dentry_by_name(filename, &dir_entry) == -1) {
		return -1;
	}

	/* Get current PCB */
	pcb_t *pcb = get_pcb_ptr();
	
	uint16_t index = 2; /* 2 for FD min because 0 and 1 is for STD IN/OUT */
	while (index < NUM_MAX_OPEN_FILES) {
		/* find a empty file index to open file and mark flag */
		if (pcb->fds[index].flags == FD_OCCUP) {
			pcb->fds[index].flags = FD_AVAIL;
			pcb->fds[index].file_position = OFFSET_START;
			break;
		} else if (index == (NUM_MAX_OPEN_FILES - 1)) {
			/* there is no avalible space in fd array */
			return -1;
		}

		index++;
	}

	/* get types and goto corresponding fops table pointser and set inode */
	switch (dir_entry.filetype) {
		case FILE_TYPE_RTC:
			if (rtc_open(filename) != SUCCESS)
				return -1;
			
			/* populate fields */
			pcb->fds[index].inode = NULL;
			pcb->fds[index].fops_ptr = fops_rtc;
			break;
		
		case FILE_TYPE_DIR:
			if (directory_open(filename) != SUCCESS)
				return -1;

			/* populate fields */
			pcb->fds[index].inode = NULL;
			pcb->fds[index].fops_ptr = fops_dir;
			break;
		
		case FILE_TYPE_FILE:
			if (file_open(filename) != SUCCESS)
				return -1;
			
			/* populate fields */
			pcb->fds[index].inode = dir_entry.inodeNumber;
			pcb->fds[index].fops_ptr = fops_file;
			break;
	}
	
	return index;
}



/* sys_read
 * DESCRIPTION: system call for read, attempt to read to only open file
 * INPUTS: fd index, buffer to write to, how many bytes to read
 * OUTPUTS: calls read function
 * RETURN VALUE: nbytes on success, -1 on failure
 * SIDE EFFECTS: none
 */
int32_t sys_read(int32_t fd, void* buf, int32_t nbytes) {
    /* Get current PCB */
	pcb_t *pcb = get_pcb_ptr();
	
	/* ERROR CATCH: check fd bounds: 0 ~ 7 and empty buf */
	if ((fd < 0 || fd > (NUM_MAX_OPEN_FILES - 1)) || (buf == NULL)) {
		return -1;
	}
	
	/* check if file is unopened */
	if (pcb->fds[fd].flags == FD_OCCUP){ 
		return -1;
	}

	/* read with correct type fops via current pcb */
	return pcb->fds[fd].fops_ptr.read(fd, (char*)buf, nbytes);
}



/* sys_write
 * DESCRIPTION: system call for write attempt to write to only open files
 * INPUTS: fd index, buffer to write to, how many bytes to write
 * OUTPUTS: calls write function 
 * RETURN VALUE: nbytes on success, -1 on failure
 * SIDE EFFECTS: none
 */
int32_t sys_write(int32_t fd, const void* buf, int32_t nbytes) {
	/* Get current PCB */
	pcb_t *pcb = get_pcb_ptr_process(terminal[cur_active_terminal].apn);
	
	/* ERROR CATCH: check fd bounds: 0 ~ 7 and empty buf */
	if ((fd < 0 || fd > (NUM_MAX_OPEN_FILES - 1)) || (buf == NULL)) {
		return -1;
	}
	
	/* check if file is unopened */
	if (pcb->fds[fd].flags == FD_OCCUP) {
		return -1;
	}

	/* write with correct type fops via current pcb */
	return pcb->fds[fd].fops_ptr.write(fd, (char *)buf, nbytes);
}



/* sys_close
 * DESCRIPTION: system call for close, attempt to close an opened file
 * INPUTS: fd index
 * OUTPUTS: free the fd
 * RETURN VALUE: 0 on success, -1 on failure
 * SIDE EFFECTS: changes file array
 */
int32_t sys_close(int32_t fd)
{
	/* ERROR CHECK: FD excluding STDIN/STDOUT 2~7 */
	if (fd < 2 || fd > (NUM_MAX_OPEN_FILES - 1)) {
		return -1;
	}

	/* Get the current PCB */
	pcb_t *pcb = get_pcb_ptr();
	
	/* ERROR CATCH: trying to close unopened files */
	if (pcb->fds[fd].flags == FD_OCCUP){
		return -1;
	}

	/* Reset FD flags to AVALIABLE */
	pcb->fds[fd].flags = 0;
	
	/* CLOSE FILE with correct type ops via current pcb */
	if (0 != pcb->fds[fd].fops_ptr.close(fd))
		return -1;
	
	/* return 0 for completion */
	return 0;
}





/* halt
 * DESCRIPTION: system call for halt, attempt to halt programs that have 
                een opened, and is not the shell
 * INPUTS: status
 * OUTPUTS: system halted
 * RETURN VALUE: return 0 for success
 * SIDE EFFECTS: system stop, return to parent
 */
int32_t halt(uint8_t status)
{   
	/* declare local variables */ 
	int i;
	
	/* start critical section: stop interrupts ============================ */
	cli();

    /* Obtain PCB of current process and parent process */
    pcb_t* current_pcb = get_pcb_ptr_process(terminal[cur_active_terminal].apn);
    pcb_t* parent_pcb = get_pcb_ptr_process(current_pcb->parent_process_number);

	/* Free spot in pid_array */
    pid_array[(uint8_t)current_pcb->process_number] = FREE;

	
    /* Update all flags in PCB to default -- aka free */
 	for (i = 0; i < NUM_MAX_OPEN_FILES; i++)
 	{
 		if(current_pcb->fds[i].flags == FD_AVAIL){
 			sys_close(i);
 		}
		current_pcb->fds[i].fops_ptr = fops_error;
 		current_pcb->fds[i].flags = FD_OCCUP;
 	}
	

	/* Set executing terminal's active process number to parent_pcb (to RESTORE to) */
	terminal[cur_active_terminal].apn = parent_pcb->process_number;
	

	/* Make sure we do not halt last process active */
	if (current_pcb->process_number == current_pcb->parent_process_number )
	{
		/* Reopen SHELL if it is the last active process that we are halting */
		terminal[cur_active_terminal].active = FD_OCCUP;
		execute((uint8_t *)"shell");
	}

    /* Remap the page */
    page_remap(_128MB, _8MB + parent_pcb->process_number * _4MB);
    
    /* Reset ESP0 in TSS */
	tss.esp0 = current_pcb->parent_ksp;
	
	sti();
	/* end critical section: start interrupts ============================ */



    /* Return from IRET with Assembly */
    asm volatile(
				 ""
                 "mov %0, %%eax;"
                 "mov %1, %%esp;"
                 "mov %2, %%ebp;"
                 "jmp RETURN_FROM_IRET;"
                 :                      /* OUTPUT: none */
                 :"r"((uint32_t)status), "r"(current_pcb->parent_ksp), "r"(current_pcb->parent_kbp)   /* INPUTS: parent's status */
                 :"%eax"                 /* reg to fill */
                 );

    /* return but should not return for halt */
	return 0;
}


/* execute
 * DESCRIPTION: system call for execute, execute the process: parse command given and 
 *				see if it is executable if yes then we will have to remap the virtual memory,
 *				and load the file. Then update the PCB and Context Switch.
 * INPUTS: command as character array
 * OUTPUTS: sets new page directory and will set program in memory
 * RETURN VALUE: return 0 for success, else return -1 for fail
 * SIDE EFFECTS: modifies the PCB
 */
int32_t execute(const uint8_t* command) {
	/* start critical section: stop interrupts ============================ */
	int i;
	cli();

	/* DECLARE LOCAL VARIABLES */
	int8_t parsed_cmd[MAX_COMMAND_SIZE];
	int8_t arg[MAX_BUFFER_SIZE];
	uint8_t buffer[READ_BUFFER_SIZE];


	/* Parse command for program name */
	uint8_t cmd_end = 0;
	uint8_t cmd_start = 0;

	/* take care of leading spaces */
	while (command[cmd_start] == ' ')
		cmd_start++;

	cmd_end = cmd_start;

	/* take program name */
	while (command[cmd_end] != ' ' && command[cmd_end] != ASCII_NL && command[cmd_end] != '\0')
		cmd_end++;
	
	/* take care of trailing zeroes */
	for (i = cmd_start; i < cmd_end; i++)
		parsed_cmd[i - cmd_start] = (int8_t)command[i];
	parsed_cmd[cmd_end] = '\0';
	


	/* Parse command for arguments */
	cmd_start = cmd_end + 1;
	cmd_end = cmd_start;

	/* take arguments */
	while (command[cmd_end] != ' ' && command[cmd_end] != ASCII_NL && command[cmd_end] != '\0')
		cmd_end++;
	
	/* store argument into command */
	for (i = cmd_start; i < cmd_end; i++)
		arg[i - cmd_start] = (int8_t)command[i];
	
	arg[cmd_end-cmd_start] = '\0';
	


	/* Handle "exit" cases to close program by calling halt */
	if (strncmp("exit", parsed_cmd, READ_BUFFER_SIZE) == 0)
	{
		asm volatile(
            "pushl	$0;"
            "pushl	$0;"
            "pushl	%%eax;"
            "call halt;"
			:
			);	
	}
	else if (strncmp("term_num", parsed_cmd, READ_BUFFER_SIZE) == 0)
	{
		printf("TERM %d\n", cur_term);
	}
	


	/* check if the file is executable */
	dentry_t test_dentry;
	if (0 != read_dentry_by_name((uint8_t*)parsed_cmd, &test_dentry)){
        return -1;
	}
	


    /* ELF checking with the first 4B */
    read_data(test_dentry.inodeNumber, 0, buffer, READ_BUFFER_SIZE);
    if ( (buffer[1] != ASCII_E) || (buffer[2] != ASCII_L) || 
    	 (buffer[3] != ASCII_F) || (buffer[0] != ASCII_DEL)){
    		return -1;
    }
		

	 
    /* Entry Point Offset 24B -> 27B */
    read_data(test_dentry.inodeNumber, LOCATION_FOR_ENTRY, buffer, READ_BUFFER_SIZE);
    
    uint32_t entry_location = *((uint32_t*)buffer);
    
	int32_t new_process_num;
	/* fetch new process number */
	new_process_num = get_proc_num();
	
	/* process executing is full already */
    if (new_process_num == -1){
    	return -1;
    }

	/* Initializing pcb ptr with process number */
 	pcb_t * process_control_block = get_pcb_ptr_process(new_process_num);
	

	/* Saving ESP and EBP into the current PCB */
	asm volatile("			\n\
				movl %%ebp, %%eax 	\n\
				movl %%esp, %%ebx 	\n\
			"
			:"=a"(process_control_block->parent_kbp), "=b"(process_control_block->parent_ksp));

	

	/* fix paging */
    /* Map a new page in virtual address 0x8000000 to physical address 0x800000 */
	page_remap(_128MB, _8MB + new_process_num * _4MB);


	/* read and load file's data into the virtual memory (0x08048000) */
    read_data(test_dentry.inodeNumber, 0, (uint8_t*)LOAD_ADDRESS, LARGE_NUMBER);


	/* PCB block setup with parent pcb */
		pcb_t * parent_PCB;

 	/* set up current pcb with process number */
 	process_control_block->process_number = new_process_num;

 	/* ERROR CHECK: trying to close the first program in terminal */
	if (terminal[cur_term].active == FREE)
	{
		/* update process status, set self as parent and mark active */
		cur_active_terminal = cur_term;
		process_control_block->parent_process_number = process_control_block->process_number;
		terminal[cur_term].active = AVIL;
	}
	/* update parent's process number */
	else {
		parent_PCB = get_pcb_ptr_process(terminal[cur_term].apn);
		process_control_block->parent_process_number = parent_PCB->process_number;
	}


	/* PCB's arg is stored */
	strcpy(process_control_block->argbuf, arg);
	

 	/* initialize fd */
 	for (i = 0; i < NUM_MAX_OPEN_FILES; i++) {
 		/* set the function pointers to return -1 */
		process_control_block->fds[i].fops_ptr = fops_error;
 		/* set -1 we use inode */
 		process_control_block->fds[i].inode = -1; 
 		/* set offset to 0 */
		process_control_block->fds[i].file_position = OFFSET_START;
		/* set flag to avaliable */
 		process_control_block->fds[i].flags = FD_OCCUP;
 	}
	
	

	/* initialize STDIN/STDOUT */
	process_control_block->fds[0].fops_ptr = fops_STDIN;
	process_control_block->fds[1].fops_ptr = fops_STDOUT;
	process_control_block->fds[0].flags = FD_AVAIL; 
	process_control_block->fds[1].flags = FD_AVAIL;


	/* Update the terminal number and process number to the actve one */
	process_control_block->term = &terminal[cur_term];
	terminal[cur_term].apn = process_control_block->process_number;


    /* CONTEXT SWITCH: Save SS0 and ESP0 */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB - _8KB * (new_process_num) - 4;

    sti();
	/* end critical section: start interrupts ============================ */


    /* Restore STACK, IRET */
    asm volatile(
                 "cli;"
                 "mov $0x2B, %%ax;"
                 "mov %%ax, %%ds;"
                 "movl $0x83FFFFC, %%eax;"
                 "pushl $0x2B;"
                 "pushl %%eax;"
                 "pushfl;"
                 "popl %%edx;"
                 "orl $0x200, %%edx;"
                 "pushl %%edx;"
                 "pushl $0x23;"
                 "pushl %0;"
                 "iret;"
                 "RETURN_FROM_IRET:;"
                 "LEAVE;"
                 "RET;"
                 :						/* OUTPUTS: none */
                 :"r"(entry_location)	/* INPUTS */
                 :"%edx","%eax"			/* fill up register */
                 );

    /* return but never do because of IRET */
    return 0;
}


/* 
*	getargs()
*	DESCRIPTION: reads the command line arguments 
*	INPUT: 	buf -- pointer to buffer
* 			num_bytes -- number of bytes
*	OUTPUT: 0 if success, -1 failure
*	SIDE EFFECTS: changes contents of buf
*/
int32_t getargs(uint8_t* buf, int32_t num_bytes) {
	/* if buf null return -1 */
	if (buf == NULL)
		return -1;
	/* get the pcb */
	pcb_t* pcb = get_pcb_ptr();

	//*return condition check*/
    if (pcb->argbuf[0] == '\0') return -1;

	/* copy into the argument buffer */
	strcpy((int8_t*)buf, (int8_t*)pcb->argbuf);
	return 0;
}


/* Vidmap(uint8_t ** screen_start)
*	DESCRIPTION: maps the text mode video screen into our buffer
*	INPUT: 	screen_start -- start of vidmem
*	OUTPUT: return the address on success
*	EFFECT: effects the physical video memory buffer 
*/
int32_t vidmap(uint8_t ** screen_start)
{
	/* virtual address to physical address */
	if (screen_start == NULL || screen_start == (uint8_t**)_4MB)
	{
		return -1;
	}
	/*fix paging*/
	table_remap((uint32_t)_136MB, (uint32_t)VIDEO);

	/* where screen should start */
	*screen_start = (uint8_t*)_136MB;
	return _136MB;
}


/* set_handler()
*	DESCRIPTION: none
*	INPUT: 	none
*	OUTPUT: none
*	SIDE EFFECT: none
*/
int32_t set_handler(int32_t signum, void* handler_address)
{
	return -1;
}

/* sig_return()
*	DESCRIPTION: none
*	INPUT: 	none
*	OUTPUT: none
*	SIDE EFFECT: none
*/
int32_t sigreturn(void)
{
	return -1;
}

/* 
*	get_proc_num()
*	DESCRIPTION: gets the next available process number using the process id array
*	INPUT: none
*	OUTPUT: returns the next available process number upon success, -1 upon failure
*	SIDE EFFECTS: none
*/
int32_t get_proc_num()
{
	/* get next process number that is avaliable */
    int32_t i;
    for (i = 0; i < NUM_MAX_PROCESSES; i++) 
    {
        if (pid_array[i] == 0) 
        {
        	pid_array[i] = 1;
	    	return i;
        }
    }
    /* Return -1 if there is no more processes available */
    printf("Too many processes active. ");
    return -1;
}

/*
*	get_pcb_ptr()
*	DESCRIPTION: fetches a pointer to the PCB
*	INPUT: none
*	OUTPUT: pointer to the PCB
*	SIDE EFFECT: none
*/
pcb_t* get_pcb_ptr(void)
{
	pcb_t* ptr;
	asm volatile("				   \n\
				andl %%esp, %%eax  \n\
				"
				:"=a"(ptr)
				:"a"(PCB_PTR_MASK)
				:"cc"
				);
	return ptr;
};

/*
*	get_pcb_ptr()
*	DESCRIPTION: gets a pointer to the current pcd struct
*	INPUTS: none
*	OUPUTS: pcb pointer
*	SIDE EFFFECGTS: none
*/
pcb_t * get_pcb_ptr_process(uint32_t process)
{
	return (pcb_t *)(_8MB - (process + 1) * _8KB);
}


/*
*	has_error()
*	DESCRIPTION: none
*	INPUT: none
*	OUTPUT: returns -1
*	SIDE EFFECTS: none
*/
int32_t has_error()
{
    return -1;
}
