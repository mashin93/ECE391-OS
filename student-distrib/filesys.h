#ifndef FILESYS_H_
#define FILESYS_H_

#include "types.h"

// Constants
#define DENTRYSIZE 64       // size in bytes of dir entry
#define NAMESIZE 32

#define BLOCK_SIZE 4096     // size of a data block in bytes
#define INODE_BYTE_OFFSET 4
#define DATA_BLOCK_BYTE_OFFSET 8

// Global vars
unsigned int FILESYSLOC;

// Main functions
int32_t file_open(const uint8_t* filename);
int32_t file_read(int32_t fd, void* buf, int32_t nbytes);
int32_t file_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t file_close(int32_t fd);
int32_t directory_open(const uint8_t* filename);
int32_t directory_read(int32_t fd, void* buf, int32_t nbytes);
int32_t directory_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t directory_close(int32_t fd);

// Helper fuctions
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

#endif

