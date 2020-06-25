#ifndef PAGING_H
#define PAGING_H

/* include files for types and also extern functions */
#include "lib.h"
#include "types.h"

/* ============================== VARIABLE DECLARATIONS ======================START= */
/* video memory address location described in lib.c */
#define VM_START        0x0B8
/* number of entries in page directory and page table (4kb size) */
#define NUM_ENTRIES     1024
/* bits to align paging to */
#define ALIGN_BITS	    4096
#define CONVERT_4MB     0x400000

#define PDE_IDX_SHIFT               22          /* Page directory index in virt. address        */
#define PAGE_TABLE_PRESENT_ENTRY    7           /* USER/READ+WRITE/PRESENT                      */
#define VIDEO                       0xB8000     /* Address of video memory page                 */

/* declare global page directory array */
extern uint32_t page_directory[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));
/* declare global page table for 0MB ~ 4MB (1024 entries) */
extern uint32_t page_table[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));
/* declare global page table for 128MB ~ 132MB (1024 entries) */
extern uint32_t video_pages[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));
/* =============================================================================END= */


/* ============================== FUNCTION DECLARATIONS ======================START= */
/* funtion to initialize pages in 0MB ~ 4MB(video memory) & 4MB ~ 8MB (kernal)  */
void paging_init();
/* maps 4MB page to physical address given virtual one */
void page_remap(uint32_t virt_addr, uint32_t phys_addr);
/* maps 4MB page to vidmem's page table */
void vidmem_remap(uint32_t virt_addr, uint32_t phys_addr);
/* maps 4MB page to user's page table */
void table_remap(uint32_t virt_addr, uint32_t phys_addr);
/* find 4MB page given virtual and physical addr */
void table_to_page_mapping(uint32_t virt_addr, uint32_t phys_addr, uint32_t cur_page);
/* get rid of old info in tlb */
void flush_tlb(void);
/* =============================================================================END= */

#endif


