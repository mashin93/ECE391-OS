#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "types.h"
#include "keyboard.h"

#define MAX_TERM  3

/**TERMINAL STRUCT **/
typedef struct {
    /*which terminal we are on*/
    uint8_t id;

    /*current cursor pos*/
    uint32_t pos_x;
    uint32_t pos_y;

	/*does our terminal have an active process?*/
    uint8_t active;

    /*active process number*/	
    int8_t apn;

    /*key buffer*/
    volatile uint8_t key_buffer[KEY_BUFFER_SIZE+1]; //128
    volatile uint8_t key_buffer_idx;

    /*flag*/
    volatile uint8_t eflag;

    //ptr to video memory for terminal
    uint8_t *video_mem;
} term_t;

/* variables */
extern volatile uint8_t cur_term;
extern term_t terminal[MAX_TERM];

/* Functions to be used in Terminal */
void init_terminal(void);
int32_t switch_terminal(uint8_t term_id);
int32_t save_restore(uint8_t term_id, uint8_t s_or_v);
int32_t switch_helper(uint8_t old, uint8_t new);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

#endif /* _TERMINAL_H */
