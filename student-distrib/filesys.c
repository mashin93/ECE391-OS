#include "lib.h"
#include "types.h"
#include "filesys.h"
#include "syscalls.h" //for PCB struct

//file scope vars
static int directoryLoc = 0;


/* file_open
 * DESCRIPTION: opens the file by filename
 * INPUTS: pointer to the filename
 * OUTPUTS: int fd on success, -1 on failure
 * RETURN VALUE: int fd on success, -1 on failure
 * SIDE EFFECTS: using read dentry by name
 */
int32_t file_open(const uint8_t* filename)
{
    return 0;
}

/* file_close
 * DESCRIPTION: closes the file 
 * INPUTS: the index of fd
 * OUTPUTS: 0 if success, -1 if fail
 * RETURN VALUE: 0 if success, -1 if fail
 * SIDE EFFECTS: none
 */
int32_t file_close(int32_t fd)
{
    return 0;
}

/* file_read
 * DESCRIPTION: reads the count data into the buffer
 * INPUTS: index of file descriptor, the buffer, and the number of bytes
 * OUTPUTS: nbytes if successful
 * RETURN VALUE: nybytes if successful
 * SIDE EFFECTS: none
 */
int32_t file_read(int32_t fd, void* buf, int32_t nbytes)
{
    uint32_t inode_number;

    /* Get current PCB Pointer */
    pcb_t *pcb = get_pcb_ptr();

    // Get current file position
    uint32_t offset = pcb->fds[fd].file_position;

    // Search for the file by name
    inode_number = pcb->fds[fd].inode;
    int temp = read_data(inode_number, offset, (uint8_t*)buf, nbytes);
    
    pcb->fds[fd].file_position += temp;
    
    return temp;
}

/* file_write
 * DESCRIPTION: writes to the file, except it's a read only file system so nothing
 * INPUTS: fd, pointer to buf, nbytes
 * OUTPUTS: -1
 * RETURN VALUE: -1
 * SIDE EFFECTS: none
 */
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/* directory_open
 * DESCRIPTION: opens a directory file
 * INPUTS: pointer to 8 bit filename
 * OUTPUTS:
 * RETURN VALUE: int fd on success, int -1 on failure
 * SIDE EFFECTS: uses read_dentry_by_name
 */
int32_t directory_open(const uint8_t* filename)
{
    return 0;
}

/* directory_close
 * DESCRIPTION: close the file
 * INPUTS: fd
 * OUTPUTS: 0 success, -1 fail
 * RETURN VALUE: same as output
 * SIDE EFFECTS: none
 */
int32_t directory_close(int32_t fd)
{
    return 0;
}

/* directory_read
 * DESCRIPTION: reads files by name
 * INPUTS: int32_t fd, int8_t* buf, int32_t nbytes
 * OUTPUTS: nbytes
 * RETURN VALUE: nbytes
 * SIDE EFFECTS: none
 */
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes)
{
    dentry_t dentry;

    if (read_dentry_by_index(directoryLoc, &dentry) == 0) {
        // Clear buffer
        memset(buf, '\0', 33);

        // Copy file name into buffer if directory entry can be read
        int32_t len = strlen((int8_t*)dentry.fileName);
        strncpy((int8_t*)buf, (int8_t*)dentry.fileName, len);
        directoryLoc++;

        return len;
    }

    // TEMP FIX = directoryLoc needs to be changed for checkpoint 3
    directoryLoc = 0;
    return 0;
}

/* directory_write
 * DESCRIPTION: does nothing
 * INPUTS: int32_t fd, const int8_t* buf, int32_t nbytes
 * OUTPUTS: -1
 * RETURN VALUE: 0 success, -1 fail
 * SIDE EFFECTS: none
 */
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/* read_dentry_by_name
 * DESCRIPTION: reads directory entry by name
 * INPUTS: filename
 * OUTPUTS: directory entry 
 * RETURN VALUE: none
 * SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
    unsigned int * fileSystemStart = (unsigned int *)FILESYSLOC;
    unsigned int numDirectories = *fileSystemStart;
    int i;
    int curLen;

    // Check for valid input
    int length = strlen((int8_t*)fname);
    if (length > NAMESIZE) return -1;

    for (i = 0; i < numDirectories; i++) {
        // Calculate address of directory entry
        unsigned int * directory = (unsigned int *)(FILESYSLOC + (i+1)*DENTRYSIZE);
        
        // Get name of current file
        int8_t curName[33];
        strncpy((int8_t*)curName, (int8_t*)directory, NAMESIZE);
        curName[NAMESIZE] = '\0';
        
        // Get size of name of current file
        curLen = strlen((int8_t*)(curName));
        
        // If we found it, copy data and return
        if (length == curLen && strncmp((int8_t*)fname, (int8_t*)curName, length) == 0)
        {
            strncpy((int8_t*)(dentry->fileName), (int8_t*)curName, NAMESIZE);
            dentry->fileName[NAMESIZE] = '\0';
            dentry->filetype = *((unsigned int *)((unsigned int)directory + NAMESIZE));
            dentry->inodeNumber = *((unsigned int *)((unsigned int)directory + NAMESIZE + sizeof(unsigned int)));
            return 0;
        }
    }

    return -1;
}

/* read_dentry_by_index
 * DESCRIPTION: reads directory entry by index
 * INPUTS: index, dentry
 * OUTPUTS: directory entry by index
 * RETURN VALUE: 0 on success, -1 fail
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
    unsigned int * fileSystemStart = (unsigned int *)FILESYSLOC;
    unsigned int numDirectories = *fileSystemStart;
    
    // Check for valid index
    if (index < 0 || index >= numDirectories) return -1;

    //calculate address of directory entry
    unsigned int * directory = (unsigned int *)(FILESYSLOC + (index +1)*DENTRYSIZE);
    
    strncpy((int8_t*)(dentry->fileName), (int8_t*)directory, NAMESIZE);
    dentry->fileName[NAMESIZE] = '\0';
    dentry->filetype = *((unsigned int *)((unsigned int)directory + NAMESIZE));
    dentry->inodeNumber = *((unsigned int *)((unsigned int)directory + NAMESIZE + sizeof(unsigned int)));

    return 0;
}

/* read_data
 * DESCRIPTION: read data
 * INPUTS: inode , offset, buffer, length
 * OUTPUTS: daata into buf
 * RETURN VALUE: nbytes
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
    uint32_t byte_count = 0;
    uint8_t * boot_block_ptr = (uint8_t *)FILESYSLOC;
    uint32_t total_inodes = *((uint32_t *)(boot_block_ptr + INODE_BYTE_OFFSET)); // Get the total number of inodes

    // Return -1 if inode number is invalid
    if (inode < 0 || inode >= total_inodes) return -1;

    uint32_t total_data_blocks = *((uint32_t *)(boot_block_ptr + DATA_BLOCK_BYTE_OFFSET)); // Get the total number of data blocks
    uint8_t * inode_ptr = boot_block_ptr + BLOCK_SIZE*(inode + 1); // Set a pointer to the relevant inode
    uint8_t * data_block_start_ptr = boot_block_ptr + BLOCK_SIZE*(total_inodes + 1); // Set a pointer to the start of the data blocks
    uint32_t file_size = *((uint32_t *)(inode_ptr)); // Get the file size in bytes

    // Return 0 if offset is at or past the end of file
    if (offset >= file_size) return 0;
    
    // If more bytes are requested than available, cut the number of bytes requested
    if (length + offset > file_size)
        length = file_size - offset;

    // Set a pointer to the first relevant data block index in inode
    uint32_t * data_block_index_ptr_in_inode = (uint32_t *)(inode_ptr + (offset/BLOCK_SIZE + 1)*INODE_BYTE_OFFSET);
    
    // Return -1 if data block number is invalid
    if (*data_block_index_ptr_in_inode < 0 || *data_block_index_ptr_in_inode >= total_data_blocks) 
        return -1;
    
    // Handle the first data block
    uint8_t * data_ptr = data_block_start_ptr + BLOCK_SIZE*(*data_block_index_ptr_in_inode) + offset%BLOCK_SIZE;//set pointer to start of data to copy
    if (offset%BLOCK_SIZE + length <= BLOCK_SIZE )
    {
        memcpy(buf, data_ptr, length);  // Copy length bytes into buf
        byte_count += length;
        return byte_count;
    }

    // Copy (BLOCK_SIZE - offset) bytes into buf and change length and buf accordingly
    memcpy(buf, data_ptr, BLOCK_SIZE - (offset%BLOCK_SIZE));
    length -= BLOCK_SIZE - (offset%BLOCK_SIZE);
    buf += BLOCK_SIZE - (offset%BLOCK_SIZE);
    byte_count += BLOCK_SIZE - (offset%BLOCK_SIZE);

    // Process the rest of the data blocks
    while (length > 0)
    {
        data_block_index_ptr_in_inode++; // increment data_block_index ptr

        // Return -1 if data block number is invalid        
        if (*data_block_index_ptr_in_inode < 0 || *data_block_index_ptr_in_inode >= total_data_blocks)
            return -1;
    
        data_ptr = data_block_start_ptr + BLOCK_SIZE*(*(data_block_index_ptr_in_inode));
        if (length <= BLOCK_SIZE) {
            memcpy(buf, data_ptr, length); // Copy length bytes into buf
            byte_count += length;

            return byte_count;
        }

        // Copy BLOCK_SIZE bytes into buf and change length and buf accordingly
        memcpy(buf, data_ptr, BLOCK_SIZE);
        length -= BLOCK_SIZE;
        buf += BLOCK_SIZE;
        byte_count += BLOCK_SIZE;
    }
    return 0;
}
