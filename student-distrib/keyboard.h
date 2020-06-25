#ifndef KEYBOARD_H
#define KEYBOARD_H

/* The following keyboard definition maps to scan codes */
#define   CTRL_C          3
#define   CTRL_L         12
#define   BS    	      8
#define   ENTER		     13
#define   ESC		     27
#define   SPACEBAR	     32
#define   TAB	          9

#define   SHIFT_TAB	    271
#define   INS		    338
#define   DEL		    339
#define   CAP            13
#define   SHI            39
#define   IFT            34

#define   CTRL_LEFT	    371
#define   CTRL_RIGHT    372

#define   ALT_SPACE	    510
#define   CTRL_SPACE	511
#define   LSHIFT_SPACE	512
#define   RSHIFT_SPACE	513

//#ifndef ASM

#define KEYBOARD_DATA_PORT 0x60     /* location of keybord data */
#define KEYBOARD_CTRL_PORT 0x61     /* location of keybord port */

#define KEY_BUFFER_SIZE		127
#define TERMINAL_ONE 0
#define TERMINAL_TWO 1
#define TERMINAL_THREE 2

extern volatile uint8_t key_buffer_idx;
extern volatile uint8_t eflag;
extern volatile uint8_t *key_buffer;

/* these three functions are defined in interrupt.S */

/* enable keyboard input */
void init_keyboard(void);

/* processing keyboard input */
void keyboard_interrupt_handler(void);

/* dummy method so far, will implement more later */
//void handle_trap(void);

/* given a buffer, putc on to terminal, following the rules 
*  provided to us (CTRL-L clears, enters properly, backspaces)
*/
// extern int getline(char *buf, int max);

/* helper function to pass the characters read to getline function */
// char getchar(void);

/* emulation of terminal -- more to write soon */
//void start_terminal(void);

// #define   CTRL_A     1
// #define   CTRL_B     2
// #define   CTRL_C     3
// #define   CTRL_D     4
// #define   CTRL_E     5
// #define   CTRL_F     6
// #define   CTRL_G     7
// #define   CTRL_H     8
// #define   CTRL_I     9
// #define   CTRL_J    10
// #define   CTRL_K    11
// #define   CTRL_L    12
// #define   CTRL_M    13
// #define   CTRL_N    14
// #define   CTRL_O    15
// #define   CTRL_P    16
// #define   CTRL_Q    17
// #define   CTRL_R    18
// #define   CTRL_S    19
// #define   CTRL_T    20
// #define   CTRL_U    21
// #define   CTRL_V    22
// #define   CTRL_W    23
// #define   CTRL_X    24
// #define   CTRL_Y    25
// #define   CTRL_Z    26

// #define   ALT_1     376
// #define   ALT_2 	377
// #define   ALT_3 	378
// #define   ALT_4 	379
// #define   ALT_5 	380
// #define   ALT_6 	381
// #define   ALT_7 	382
// #define   ALT_8 	383
// #define   ALT_9 	384
// #define   ALT_0	    385
// #define   ALT_HYP	386
// #define   ALT_EQU	387

// #define   ALT_Q     272
// #define   ALT_W 	273
// #define   ALT_E 	274
// #define   ALT_R 	275
// #define   ALT_T 	276
// #define   ALT_Y 	277
// #define   ALT_U 	278
// #define   ALT_I 	279
// #define   ALT_O 	280
// #define   ALT_P  	281

// #define   ALT_A     286
// #define   ALT_S 	287
// #define   ALT_D 	288
// #define   ALT_F 	289
// #define   ALT_G 	290
// #define   ALT_H 	291
// #define   ALT_J 	292
// #define   ALT_K 	293
// #define   ALT_L 	294

// #define   ALT_Z     290
// #define   ALT_X 	291
// #define   ALT_C 	292
// #define   ALT_V 	293
// #define   ALT_B 	294
// #define   ALT_N 	295
// #define   ALT_M 	296

//#endif // closing asm

void handle_key_press(uint16_t keycode) ;
void add_key_buff(uint8_t key);
void clear_buf();

#endif // closing keyboard
