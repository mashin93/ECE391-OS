#include "paging.h"

/* ============================== GLOBAL PAGE DIRECTORIES  ======================START= */
/* declare global page directory array */
uint32_t page_directory[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));

/* declare global page table for 0MB ~ 4MB (1024 entries) */
uint32_t page_table[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));

/* declare global user page table (1024 entries) */
uint32_t user_page_table[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));

/* declare global video memory page table for 128MB ~ 132MB (1024 entries) */
uint32_t video_page_table[NUM_ENTRIES] __attribute__((aligned(ALIGN_BITS)));
/* =============================================================================END= */

/* paging_init
 *   DESCRIPTION: Called by kernel.c to initialize paging. 
 *                Sets up 4kb pages for first 4MB including video memory, and 
 *                Sets up one page of 4MB for kernel, location defined in makefile. 
 *                Other memory will be marked as NOT "present".
 *   INPUT: none
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: Arrays for page_directory and page_table is declared and initialized.
 *                Bits for each pages initialized to 0, sttribute bits marked present correspondingly.
 *                Change the control registers bits to enable paging after control registers are set.
 */
void paging_init() {
    /* DEBUG:   printf(" Paging Initialization! \n"); */

    /* initialize page directory to contain page tables with correct attributes */
    unsigned int i;
    for(i = 0; i < NUM_ENTRIES; i++) {
        /* 0x02 = 10 mark each page tables in directory as read/write & not present
         * NOTE: only kernel-mode can access them (supervisor) from os dev */
        page_directory[i] = 0x00000002;
    }
    
    /* initialize page table for 0MB ~ 4MB containing video memory */
    for (i = 0; i < NUM_ENTRIES; i++) {
        if (i >= VM_START && i <= VM_START + 3) {           /* video memory is 4 bytes (0 ~ 3 so +3) */
            page_table[i] = (i * 0x1000) | 3;               /* 0x03 = 11 read/write, mark vid mem as present */
        }                                                   /* 12 bits skipped: 0x1000 */
        else{
            page_table[i] = (i * 0x1000) | 2;               /* 0x02 = 10 read/write, mark non vid mem as not present */
        }
    }

    /* set first page table address and also mar read/write and as present on first entry of page_directory */
    page_directory[0] = ((unsigned int)page_table) | 3;     /* 0x03 = 11 read/write, mark vid mem as present */


    /* set second entry in page directories to point to kernel 4MB page
     * 0x3(11): mark supervisor: access kernel, read/write, present
     * 0x80(1000 0000): mark as 4MB page size for kernel entry
     * 0x400000: address to 4MB the start of kernel page defined by makefile
     */
    page_directory[1] = 0x00400083;
    
    /* set control registers to initialize paging */
    asm volatile(
        /* load page_directory address to cr3 */
        "movl %0, %%eax;"
        "movl %%eax, %%cr3;"

        /* enable Page Size Entension(bit4) in cr4 for 4MB page */
        "movl %%cr4, %%eax;"
        "orl $0x00000010, %%eax;"
        "movl %%eax, %%cr4;"

        /* enable paging(bit31) and protected mode enable(bit0) in cr0 */
        "movl %%cr0, %%eax;"
        "orl $0x80000000, %%eax;"
        "movl %%eax, %%cr0;"
                
        :                           /* OUTPUT: none */
        :"r"(page_directory)        /* INPUT: page_directory */
        :"%eax"                     /* register to fill*/
    );
}


/* page_remap
 *   DESCRIPTION: Maps a PDE (page directory entry) to the start of the virtual 
 *                address given with the physical address given.
 *   INPUT: virt_addr -- the 4MB virtual address that function will map
 *          phys_addr -- physical address to be mapped
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: PDE is mapped virually and physically.
 */
void page_remap(uint32_t virt_addr, uint32_t phys_addr) {
    uint32_t page_dir_entry = virt_addr / CONVERT_4MB;
    
    /*  set page directory entry attributes:
        - USER/READ+WRITE/PRESENT
        - 0x80(1000 0000): mark as 4MB page size for kernel entry  */
    page_directory[page_dir_entry] = phys_addr | 0x87;
    
    /* reset the tlb */
    flush_tlb();
}

/* table_remap
 *   DESCRIPTION: Maps a PDE (page directory entry) to the start of 
 *                user page table's first entry.
 *   INPUT: virt_addr -- the 4MB virtual address that function will map
 *          phys_addr -- physical address to be mapped
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: PDE is mapped virually and physically and updated user's page table.
 */
void table_remap(uint32_t virt_addr, uint32_t phys_addr) {
    uint32_t page_dir_entry = virt_addr / CONVERT_4MB;
    
    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT  */
    page_directory[page_dir_entry] = ((unsigned int)user_page_table) | PAGE_TABLE_PRESENT_ENTRY;
    
    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT  */
    user_page_table[0] = phys_addr | PAGE_TABLE_PRESENT_ENTRY;
    
    /* reset the tlb */
    flush_tlb();
}

/* vidmem_remap
 *   DESCRIPTION: Maps a PDE (page directory entry) to the start of 
 *                video memory page table's first entry.
 *   INPUT: virt_addr -- the 4MB virtual address that function will map
 *          phys_addr -- physical address to be mapped
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: PDE is mapped virually and physically and update video memory page table.
 */
void vidmem_remap(uint32_t virt_addr, uint32_t phys_addr) {
    uint32_t page_dir_entry = virt_addr / CONVERT_4MB;

    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT */
    page_directory[page_dir_entry] = ((unsigned int)video_page_table) | PAGE_TABLE_PRESENT_ENTRY;
    
    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT  */
    video_page_table[0] = phys_addr | PAGE_TABLE_PRESENT_ENTRY;
    
    /* reset the tlb */
    flush_tlb();
}

/* table_to_page_mapping
 *   DESCRIPTION: Maps a PDE (page directory entry) to specific page in user's page table.
 *   INPUT: virt_addr -- the 4MB virtual address that function will map
 *          phys_addr -- physical address to be mapped
 *          cur_page --  holds the index to the current specific page that we need to remap
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: PDE is mapped virually and physically and update specific page in user's page table.
 */
void table_to_page_mapping(uint32_t virt_addr, uint32_t phys_addr, uint32_t cur_page) {
    uint32_t page_dir_entry = virt_addr / CONVERT_4MB;

    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT  */
    page_directory[page_dir_entry] = ((unsigned int)user_page_table) | PAGE_TABLE_PRESENT_ENTRY;
    
    /* set page directory entry attributes:
       USER/READ+WRITE/PRESENT  */
    user_page_table[cur_page] = phys_addr | PAGE_TABLE_PRESENT_ENTRY;
    
    /* reset the tlb */
    flush_tlb();
}


/* flush_tlb
 *   DESCRIPTION: Flush the tlb by changing the 3rd Control Register. But maybe don't want to alter CR3 value?
 *   INPUT: none
 *   OUTPUT: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: CR3 = CR3
 */
void flush_tlb(void){
	asm volatile(
        "mov %%cr3, %%eax;"
        "mov %%eax, %%cr3;"
                 
        :       /* output: none */
        :       /* input: none */
        :"%eax" /* register to fill */
    );
}
