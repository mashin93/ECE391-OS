#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "x86_desc.h"
#include "lib.h"
#include "types.h"
#include "paging.h"
#include "rtc.h"
#include "filesys.h"
#include "terminal.h"
#include "syscalls.h"

#define OPEN 0
#define READ 1
#define WRITE 2
#define CLOSE 3

#define NUM_MAX_PROCESSES 6
#define NUM_MAX_OPEN_FILES 8

#define PROG_NOT_ACTIVE 0
#define PROG_ACTIVE 1

#define FD_OCCUP 0
#define FD_AVAIL 1

#define OFFSET_START 0
#define FREE 0
#define AVIL 1

#define SUCCESS 0
#define FAILED -1

#define _128MB 	0x8000000
#define _8MB 	0x800000
#define _4MB 	0x400000
#define _4KB 	0x1000
#define _4B 	4

#define TERMINAL_BUFFER_SIZE	128
#define MAX_COMMAND_SIZE 256
#define MAX_BUFFER_SIZE 100
#define READ_BUFFER_SIZE 4
#define LOCATION_FOR_ENTRY 24


#define PCB_PTR_MASK 0xFFFFE000 
#define LARGE_NUMBER 100000
#define LOAD_ADDRESS 0x8048000
#define IN_USE 0x0001
#define NOT_IN_USE 0x0000
#define FILE_START 0x0000

#define FILE_TYPE_RTC	0 
#define FILE_TYPE_DIR	1
#define	FILE_TYPE_FILE	2

#define FILE_NAME_SIZE 32


/*struct for defining fops*/
 typedef struct {
	 int32_t (*read)(int32_t fd, void* buf, int32_t nbytes);
	 int32_t (*write)(int32_t fd, const void* buf, int32_t nbytes);
	 int32_t (*open)(const uint8_t* filename);
	 int32_t (*close)(int32_t fd);
 } fops_t;
 
 
/*struct for defining file descriptor*/

typedef struct { 
	fops_t fops_ptr; 
	int32_t inode; 
	int32_t file_position; 
	int32_t flags; 
} file_desc_t;

/*struct for defining pcb*/

typedef struct { 
	file_desc_t fds[NUM_MAX_OPEN_FILES]; 
	uint8_t filenames[NUM_MAX_OPEN_FILES][FILE_NAME_SIZE];  
	uint32_t parent_ksp; 
	uint32_t parent_kbp; 
	uint8_t process_number; 
	uint8_t parent_process_number; 
	int8_t argbuf[MAX_BUFFER_SIZE]; 
	term_t * term;
    uint32_t esp;
    uint32_t ebp;
 } pcb_t; 
 
 extern uint8_t process_id_array [NUM_MAX_PROCESSES];

/* halt programs */
int32_t halt (uint8_t status);

/* execute programs */
int32_t execute (const uint8_t* command);

/* read programs */
int32_t sys_read (int32_t fd, void* buf, int32_t  nbytes);

/* write programs */
int32_t sys_write (int32_t fd, const void* buf, int32_t nbytes);

/* open programs */
int32_t sys_open (const uint8_t* filename);

/* close programs */
int32_t sys_close (int32_t fd);

/* get arguments */
int32_t getargs (uint8_t* buf, int32_t nbytes);

/* video memory set */
int32_t vidmap (uint8_t** screen_start);

/* handler setter */
int32_t set_handler (int32_t signum, void* handler_address);

/* signal generator */
int32_t sigreturn (void);

/* get current process pcb */
pcb_t* get_pcb_ptr();

/* get current process pcb ptr */
pcb_t* get_pcb_ptr_process(uint32_t process);

/* gets next process's number that is avaliable */
int32_t get_proc_num();

/* function failed task, give eror pointer and return -1 */
int32_t has_error();


#endif
