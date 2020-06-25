#include "terminal.h"
#include "lib.h"
#include "syscalls.h"
#include "types.h"
#include "i8259.h"
#include "paging.h"
#include "scheduler.h"
#define LOCATION	 2*NUM_ROWS*NUM_COLS

/* current terminal declaration */
volatile uint8_t cur_term;
term_t terminal[MAX_TERM];

/*
* init_terminal()
*   DESCRIPTION:  Initializes the terminal by updating all the pointers, sets up paging at that terminal  
*   INPUT:        none
*   OUTPUT:       none
*   RETURN VALUE: none
*   SIDE EFFECTS: initializes each of the terminals
*/
void init_terminal() 
{
	uint8_t i;
	uint32_t j;

	for (i = 0; i < MAX_TERM; i++) {

		terminal[i].id = i;
		terminal[i].pos_x = 0;
		terminal[i].pos_y = 0;
		terminal[i].active = 0;
		terminal[i].apn = -1;
		terminal[i].key_buffer_idx = 0;
		terminal[i].eflag = 0;

		/* fill the buffer up with keyboard */
		for (j = 0; j < KEY_BUFFER_SIZE; j++)
			terminal[i].key_buffer[j] = '\0';

		/* set paging and video mapping */
		table_to_page_mapping(_100MB, _100MB+((i+1)*_4KB), i+1);
		terminal[i].video_mem = (uint8_t *)_100MB+((i+1)*_4KB);

		/*clear memory*/
		for (j = 0; j < NUM_ROWS*NUM_COLS; j++) {
			*(uint8_t *)(terminal[i].video_mem + (j << 1)) = ' ';

			/*set color -- change it later*/
        	if (i == 0)
        		*(uint8_t *)(terminal[i].video_mem + (j << 1) + 1) = ATTRIB_TERM1;
	    	if (i == 1)
        		*(uint8_t *)(terminal[i].video_mem + (j << 1) + 1) = ATTRIB_TERM2;
	   		if (i == 2)
        		*(uint8_t *)(terminal[i].video_mem + (j << 1) + 1) = ATTRIB_TERM3;
		}
	}

	/* launch the first terminal */
	key_buffer = terminal[0].key_buffer;
	save_restore(0,1);
	cur_term = 0;
	/* always execute shell */
	execute((uint8_t*)"shell");
}

/*
* init_terminal()
*   DESCRIPTION:  switches into the new terminal, saving and restoring  
*   INPUT:        uint8_t term_num -- terminal to be switched into 
*   OUTPUT:       returns 0 on success
*   RETURN VALUE: none
*   SIDE EFFECTS: initializes each of the variables in the terminal
*/
int32_t switch_terminal(uint8_t term_num) 
{
	cli();
	/* -------------------------------------------------------------------------------------------------------------------------- 
														CRITICAL SECTION BEGIN														*/

	/* if terminal number is greater than 2, return*/
	if (term_num > MAX_TERM-1) return -1;

	/* don't have to switch to itself */
	if (term_num == cur_term) return 0;

	/* Terminal is already active - simply restoring state (keyboard buff, vidmem) */
	if (terminal[term_num].active == 1) {
		/* only switch if it can be switched */
		if (switch_helper(cur_term, term_num) == -1) return -1;

		key_buffer = terminal[term_num].key_buffer;
		cur_term = term_num;
        /* Remap video memory to 136 MB */
        uint8_t * scr_str;
        vidmap(&scr_str);
        if (terminal[cur_active_terminal].id != cur_term) {
            vidmem_remap((uint32_t)scr_str, (uint32_t)terminal[cur_active_terminal].video_mem);
        }

		return 0;
	}
	
	/* If terminal is not active, then save the current and launch new terminal */
	save_restore(cur_term,0);

	/* launch new terminal */
	cur_term = term_num;
	pcb_t * old_pcb = get_pcb_ptr_process(terminal[cur_active_terminal].apn);
	key_buffer = terminal[term_num].key_buffer;
	save_restore(term_num,1);	

    /* Save the ebp/esp of the process we are switching away from. */
    asm volatile("			\n\
                 movl %%ebp, %%eax 	\n\
                 movl %%esp, %%ebx 	\n\
                 "
                 :"=a"(old_pcb->ebp), "=b"(old_pcb->esp)
	);

	/*											CRITICAL SECTION END
	---------------------------------------------------------------------------------------------------------------------------- */
	sti();

	/* always execute shell */
	execute((uint8_t*)"shell");
	return 0;
}

/*
* save_restore(uint8_t term_num, uint8_t s_or_r)
*   DESCRIPTION:  restores or saves the state of a terminal  
*   INPUT:        uint8_t term_num -- terminal 1 2 or 3 to be saved/restored
*				  uint8_t s_or_r -- 0 for save, 1 for restore
*   OUTPUT:       0 on success
*   RETURN VALUE: 0 or -1 
*   SIDE EFFECTS: none
*/
int32_t save_restore(uint8_t term_num, uint8_t s_or_r) 
{
	if (s_or_r == 0) {
	terminal[term_num].pos_x = get_screen_x();
	terminal[term_num].pos_y = get_screen_y();
	terminal[term_num].key_buffer_idx = key_buffer_idx;
	memcpy((uint8_t *)terminal[term_num].video_mem, (uint8_t *)VIDEO, LOCATION);
	}
	else {
	key_buffer_idx = terminal[term_num].key_buffer_idx;
	set_screen_pos(terminal[term_num].pos_x, terminal[term_num].pos_y);
	memcpy((uint8_t *)VIDEO, (uint8_t *)terminal[term_num].video_mem, LOCATION);
	}
	return 0;
}


/*
*   switch_helper(uint8_t old, uint8_t new) 
*   DESCRIPTION:  Sees if the switching of the terminal can be done 
*   INPUT:        uint8_t old -- terminal to switch out from
*				  uint8_t new -- terminal to switch into 
*   OUTPUT:       0 on success
*   RETURN VALUE: 0 or -1 
*   SIDE EFFECTS: none
*/
int32_t switch_helper(uint8_t old, uint8_t new) 
{
	/* if switch can be done, return 0 */
	if (save_restore(old,0) == -1) return -1;
	if (save_restore(new,1) == -1) return -1;
	return 0;
}

/*
 * terminal_read()
 * DESCRIPTION: Reads from the keyoard input buffer and copies to buffer in userspace
 * INPUTS: -- int32_t fd: file directory
 * 		   -- char *buffer: userspace buffer will hold input buffer data
 *         -- uint32_t num_bytes: maximum number of bytes to copy between buffers
 * OUTPUTS: none
 * RETURN VALUE: number of bytes copied between buffers
 * SIDE EFFECTS: fills user buffer with terminal's input buffer data
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t num_bytes) 
{
	uint8_t i;
	/* set flag equal to zero */
	while (get_pcb_ptr()->term->eflag == 0);
	get_pcb_ptr()->term->eflag = 0;
	
	/* fill the buffer */
	int8_t * buffer = (int8_t *)buf;
	/* load into buffer */
	for (i = 0; (i < num_bytes-1) && (i < KEY_BUFFER_SIZE); i++) {
		buffer[i] = key_buffer[i];
	}
	
	clear_buf();
	return i;
}

/*
 * terminal_write()
 * DESCRIPTION: writes string (buf of chars) to screen until a null character
                or the number of chars to write is reached
 * INPUTS: -- uint8_t* buffer: holding the string to write to screen
 *         -- uint32_t num_bytes: maximum number of characters to write
 * OUTPUTS: outputs to video memory
 * RETURN VALUE: number of characters wrote to video mem/screen (max at num_chars)
 * SIDE EFFECTS: writes to video memory/terminal screen
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t num_bytes) 
{
	int32_t idx;

	cli();
	/* ============= START critical section to write buffer ============= */

	if (cur_term == cur_active_terminal) {
		idx = printf((int8_t *)buf);
	}
	else {
		idx = printf_terminal_active((int8_t *)buf);

	}

	/* ============== END critical section to write buffer ============== */
	sti();

	return idx;
}

/*
 * terminal_open()
 * DESCRIPTION: called by syscalls, but no specific tasks to carryout
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: returns 0 always
 * SIDE EFFECTS: none
 */
int32_t terminal_open(const uint8_t* filename) 
{
	/* return 0 always - no tasks */
	return 0;
}

/*
 * terminal_close()
 * DESCRIPTION: called by syscalls, but no specific tasks to carryout
 * INPUTS: none
 * OUTPUTS: none
 * RETURN VALUE: returns 0 always
 * SIDE EFFECTS: none
 */
int32_t terminal_close(int32_t fd) 
{
	/* return 0 always - no tasks */
	return 0;
}


