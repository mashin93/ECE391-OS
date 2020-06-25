#include "lib.h"
#include "i8259.h"
#include "terminal.h"
#include "scheduler.h"

#define VIDEO       0xB8000                 /* video memory statrt location */
#define NUM_COLS    80                      /* number of columns of terminal screen */
#define NUM_ROWS    25                      /* number of rows of terminal screen */
#define ATTRIB      0x7                     /* attribute: black given to write in putc etc */

static int screen_x;                        /* holds the x location to putc and set cursor */
static int screen_y;                        /* holds the y location to putc and set cursor */
static char* video_mem = (char *)VIDEO;     /* converts the Vid Mem location to a pointer */
static uint8_t attribute[3] = { ATTRIB_TERM1, ATTRIB_TERM2, ATTRIB_TERM3 };

/*
* void set_cursor_pos(void);
*   Special print function to write to the currently executing terminal's video buffer
*/
void set_cursor_pos() {
	uint16_t position = NUM_COLS*screen_y + screen_x;
	outw(0x000E | (position & 0xFF00), 0x03D4);
	outw(0x000F | ((position << 8) & 0xFF00), 0x03D4);
}

/*
 * clear
 *  DESCRIPTION:    clears video memory 
 *  INPUT:          none
 *  OUTPUT:         none
 *  RETURN VALUE:   none
 *  SIDE EFFECT:    none
 */
void clear(void)
{
    /* Clear video screen as blank */
	memset_word(video_mem, attribute[cur_term] << 8 | ' ', NUM_ROWS * NUM_COLS);
}

/*
 * blue_screen
 *  DESCRIPTION:    sets screen to blue
 *  INPUT:          none
 *  OUTPUT:         none
 *  RETURN VALUE:   none
 *  SIDE EFFECT:    prints blue screen
 */
void blue_screen(void) {
    /* Clear video screen as blue*/
	int32_t i;
    for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
		*(uint8_t *)(video_mem + (i << 1) + 1) = BLUE_SCREEN;
	}
}

/*
*	Function: get_screen_x()
*	Description: Gets the current x coor
*	input: none
*	output: x coordinate
*	effects: none
*/
int get_screen_x(void) {
	return screen_x;
}

/*
*	Function: set_screen_x()
*	Description: Sets the current x coor
*	input: x coordinate
*	output: none
*	effects: changes x coord
*/
void set_screen_x(uint32_t new_x) {
	set_screen_pos(new_x, screen_y);
}

/*
*	Function: get_screen_y()
*	Description: Gets the current y coor
*	input: none
*	output: y coordinate
*	effects: none
*/
int get_screen_y(void) {
	return screen_y;
}

/*
*	Function: set_screen_y()
*	Description: Sets the current y coor
*	input: y coordinate
*	output: none
*	effects: changes y coord
*/
void set_screen_y(uint32_t new_y) {
	set_screen_pos(screen_x, new_y);
}

/*
* void set_screen_pos(uint32_t x, uint32_t y);
*   Inputs: x and y coordinates to start writing text to the screen
*   Return Value: none
*	Function: moves the location text is drawn on the screen
*/
void set_screen_pos(uint32_t x, uint32_t y) {
	if (x < NUM_COLS)
		screen_x = x;
	else {
		enter();
		set_cursor_pos();
		return;
	}

	if (y < NUM_ROWS)
		screen_y = y;
	else {
		scroll_up();
		screen_y = NUM_ROWS - 1;
	}
	set_cursor_pos();
}

/*
* void set_screen_pos_term_exec(uint32_t x, uint32_t y);
*   Special print function to write to the currently executing terminal's video buffer
*/
void set_screen_pos_term_exec(uint32_t x, uint32_t y) {
	if (x < NUM_COLS)
		terminal[cur_active_terminal].pos_x = x;
	else {
		enter_term_exec();
		return;
	}

	if (y < NUM_ROWS)
		terminal[cur_active_terminal].pos_y = y;
	else {
		scroll_up_term_exec();
		terminal[cur_active_terminal].pos_y = NUM_ROWS - 1;
	}
}

/*
* void enter(void);
*   Inputs: void
*   Return Value: none
*	Function: Moves to the next line on the screen
*/
void enter(void) {
	set_screen_pos(ROW_START, screen_y+1);
}

/*
* void enter(void);
*   Special print function to write to the currently executing terminal's video buffer
*/
void enter_term_exec(void) {
	set_screen_pos_term_exec(ROW_START, terminal[cur_active_terminal].pos_y+1);
}

/*
* void backspace(void);
*   Inputs: void
*   Return Value: none
*	Function: Removes a character from the screen
*/
void backspace(void) {
	if (screen_x == ROW_START) {
		set_screen_pos(NUM_COLS-1, screen_y-1);
	}
	else {
		set_screen_x(screen_x-1);
	}

	*(uint8_t *)(video_mem + ((NUM_COLS * screen_y + screen_x) << 1)) = ' ';
    *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = attribute[cur_term];
    // if (cur_term == TERMINAL_ONE)
    //     	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM1;
    //     if (cur_term == TERMINAL_TWO)
    //     	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM2;
    //     if (cur_term == TERMINAL_THREE)
    //     	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM3;
}

/*
* void scroll_up(void);
*   Inputs: void
*   Return Value: none
*	Function: Removes a character from the screen
*/
void scroll_up() {
	int32_t x;
	int32_t y;
	int32_t old_line;
	int32_t new_line;

	// Move each row in the video memory up by 1
	for (y = 0; y < NUM_ROWS-1; y++) {
		for (x = 0; x < NUM_COLS; x++) {
			old_line = NUM_COLS*(y+1) + x;
			new_line = NUM_COLS*y + x;
			*(uint8_t *)(video_mem + (new_line << 1)) = *(uint8_t *)(video_mem + (old_line << 1));
		}
	}

	// Clear the bottom line
	for (x = 0; x < NUM_COLS; x++) {
		new_line = NUM_COLS*(NUM_ROWS-1) + x;
		*(uint8_t *)(video_mem + (new_line << 1)) = ' ';
	}
	set_screen_x(ROW_START);
}

/*
* void scroll_up_term_exec(void);
*   Special print function to write to the currently executing terminal's video buffer
*/
void scroll_up_term_exec() {
	int32_t x;
	int32_t y;
	int32_t old_line;
	int32_t new_line;

	// Move each row in the video memory up by 1
	for (y = 0; y < NUM_ROWS-1; y++) {
		for (x = 0; x < NUM_COLS; x++) {
			old_line = NUM_COLS*(y+1) + x;
			new_line = NUM_COLS*y + x;
			*(uint8_t *)(terminal[cur_active_terminal].video_mem + (new_line << 1)) = *(uint8_t *)(terminal[cur_active_terminal].video_mem + (old_line << 1));
		}
	}

	// Clear the bottom line
	for (x = 0; x < NUM_COLS; x++) {
		new_line = NUM_COLS*(NUM_ROWS-1) + x;
		*(uint8_t *)(terminal[cur_active_terminal].video_mem + (new_line << 1)) = ' ';
	}
	set_screen_pos_term_exec(ROW_START, terminal[cur_active_terminal].pos_y);
}

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output.
 * */
int32_t printf(int8_t *format, ...)
{
	/* Pointer to the format string */
	int8_t* buf = format;

	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

					format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc( (uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc(*buf);
				break;
		}
		buf++;
	}

	return (buf - format);
}

/* Standard printf_terminal_active().
 * Special print function to write to the currently executing terminal's video buffer
 */
int32_t printf_terminal_active(int8_t *format, ...)
{
	/* Pointer to the format string */
	int8_t* buf = format;

	/* Stack pointer for the other parameters */
	int32_t* esp = (void *)&format;
	esp++;

	while(*buf != '\0') {
		switch(*buf) {
			case '%':
				{
					int32_t alternate = 0;
					buf++;

					format_char_switch:
					/* Conversion specifiers */
					switch(*buf) {
						/* Print a literal '%' character */
						case '%':
							putc_terminal_active('%');
							break;

						/* Use alternate formatting */
						case '#':
							alternate = 1;
							buf++;
							/* Yes, I know gotos are bad.  This is the
							 * most elegant and general way to do this,
							 * IMHO. */
							goto format_char_switch;

						/* Print a number in hexadecimal form */
						case 'x':
							{
								int8_t conv_buf[64];
								if(alternate == 0) {
									itoa(*((uint32_t *)esp), conv_buf, 16);
									puts_terminal_active(conv_buf);
								} else {
									int32_t starting_index;
									int32_t i;
									itoa(*((uint32_t *)esp), &conv_buf[8], 16);
									i = starting_index = strlen(&conv_buf[8]);
									while(i < 8) {
										conv_buf[i] = '0';
										i++;
									}
									puts_terminal_active(&conv_buf[starting_index]);
								}
								esp++;
							}
							break;

						/* Print a number in unsigned int form */
						case 'u':
							{
								int8_t conv_buf[36];
								itoa(*((uint32_t *)esp), conv_buf, 10);
								puts_terminal_active(conv_buf);
								esp++;
							}
							break;

						/* Print a number in signed int form */
						case 'd':
							{
								int8_t conv_buf[36];
								int32_t value = *((int32_t *)esp);
								if(value < 0) {
									conv_buf[0] = '-';
									itoa(-value, &conv_buf[1], 10);
								} else {
									itoa(value, conv_buf, 10);
								}
								puts_terminal_active(conv_buf);
								esp++;
							}
							break;

						/* Print a single character */
						case 'c':
							putc_terminal_active((uint8_t) *((int32_t *)esp) );
							esp++;
							break;

						/* Print a NULL-terminated string */
						case 's':
							puts_terminal_active( *((int8_t **)esp) );
							esp++;
							break;

						default:
							break;
					}

				}
				break;

			default:
				putc_terminal_active(*buf);
				break;
		}
		buf++;
	}

	return (buf - format);
}

/*
* int32_t puts(int8_t* s);
*   Inputs: int_8* s = pointer to a string of characters
*   Return Value: Number of bytes written
*	Function: Output a string to the console
*/
int32_t puts(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc(s[index]);
		index++;
	}

	return index;
}

/*
* void putc(uint8_t c);
*   Inputs: uint_8* c = character to print
*   Return Value: void
*	Function: Output a character to the console
*/
void putc(uint8_t c)
{
    if(c == '\n' || c == '\r') {
        enter();
    } else {
        *(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1)) = c;
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = attribute[cur_term];
        // if (cur_term == TERMINAL_ONE)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM1;
        // if (cur_term == TERMINAL_TWO)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM2;
        // if (cur_term == TERMINAL_THREE)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM3;
        set_screen_pos(screen_x+1, screen_y);
    }
}

/*
* int32_t puts(int8_t* s);
*   Inputs: int_8* s = pointer to a string of characters
*   Return Value: Number of bytes written
*	Function: Output a string to the console
*/
int32_t puts_terminal_active(int8_t* s)
{
	register int32_t index = 0;
	while(s[index] != '\0') {
		putc_terminal_active(s[index]);
		index++;
	}

	return index;
}

/*
* void putc(uint8_t c);
*   Inputs: uint_8* c = character to print
*   Return Value: void
*	Function: Output a character to the console
*/
void putc_terminal_active(uint8_t c)
{
    if(c == '\n' || c == '\r') {
        enter_term_exec();
    } else {
        *(uint8_t *)(terminal[cur_active_terminal].video_mem + ((NUM_COLS*terminal[cur_active_terminal].pos_y + terminal[cur_active_terminal].pos_x) << 1)) = c;
		*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = attribute[cur_term];
        // if (cur_term == TERMINAL_ONE)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM1;
        // if (cur_term == TERMINAL_TWO)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM2;
        // if (cur_term == TERMINAL_THREE)
        // 	*(uint8_t *)(video_mem + ((NUM_COLS*screen_y + screen_x) << 1) + 1) = ATTRIB_TERM3;
        set_screen_pos_term_exec(terminal[cur_active_terminal].pos_x+1, terminal[cur_active_terminal].pos_y);
    }
}

/*
* int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix);
*   Inputs: uint32_t value = number to convert
*			int8_t* buf = allocated buffer to place string in
*			int32_t radix = base system. hex, oct, dec, etc.
*   Return Value: number of bytes written
*	Function: Convert a number to its ASCII representation, with base "radix"
*/
int8_t* itoa(uint32_t value, int8_t* buf, int32_t radix)
{
	static int8_t lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	int8_t *newbuf = buf;
	int32_t i;
	uint32_t newval = value;

	/* Special case for zero */
	if(value == 0) {
		buf[0]='0';
		buf[1]='\0';
		return buf;
	}

	/* Go through the number one place value at a time, and add the
	 * correct digit to "newbuf".  We actually add characters to the
	 * ASCII string from lowest place value to highest, which is the
	 * opposite of how the number should be printed.  We'll reverse the
	 * characters later. */
	while(newval > 0) {
		i = newval % radix;
		*newbuf = lookup[i];
		newbuf++;
		newval /= radix;
	}

	/* Add a terminating NULL */
	*newbuf = '\0';

	/* Reverse the string and return */
	return strrev(buf);
}

/*
* int8_t* strrev(int8_t* s);
*   Inputs: int8_t* s = string to reverse
*   Return Value: reversed string
*	Function: reverses a string s
*/
int8_t* strrev(int8_t* s)
{
	register int8_t tmp;
	register int32_t beg=0;
	register int32_t end=strlen(s) - 1;

	while(beg < end) {
		tmp = s[end];
		s[end] = s[beg];
		s[beg] = tmp;
		beg++;
		end--;
	}

	return s;
}

/*
* uint32_t strlen(const int8_t* s);
*   Inputs: const int8_t* s = string to take length of
*   Return Value: length of string s
*	Function: return length of string s
*/
uint32_t strlen(const int8_t* s)
{
	register uint32_t len = 0;
	while(s[len] != '\0')
		len++;

	return len;
}

/*
* void* memset(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive bytes of pointer s to value c
*/
void* memset(void* s, int32_t c, uint32_t n)
{
	c &= 0xFF;
	asm volatile("                  \n\
			.memset_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memset_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memset_aligned \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memset_top     \n\
			.memset_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     stosl           \n\
			.memset_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memset_done    \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			subl    $1, %%edx       \n\
			jmp     .memset_bottom  \n\
			.memset_done:           \n\
			"
			:
			: "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_word(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set lower 16 bits of n consecutive memory locations of pointer s to value c
*/
void* memset_word(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosw           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memset_dword(void* s, int32_t c, uint32_t n);
*   Inputs: void* s = pointer to memory
*			int32_t c = value to set memory to
*			uint32_t n = number of bytes to set
*   Return Value: new string
*	Function: set n consecutive memory locations of pointer s to value c
*/
void* memset_dword(void* s, int32_t c, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			rep     stosl           \n\
			"
			:
			: "a"(c), "D"(s), "c"(n)
			: "edx", "memory", "cc"
			);

	return s;
}

/*
* void* memcpy(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of copy
*			const void* src = source of copy
*			uint32_t n = number of byets to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of src to dest
*/
void* memcpy(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			.memcpy_top:            \n\
			testl   %%ecx, %%ecx    \n\
			jz      .memcpy_done    \n\
			testl   $0x3, %%edi     \n\
			jz      .memcpy_aligned \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%ecx       \n\
			jmp     .memcpy_top     \n\
			.memcpy_aligned:        \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			movl    %%ecx, %%edx    \n\
			shrl    $2, %%ecx       \n\
			andl    $0x3, %%edx     \n\
			cld                     \n\
			rep     movsl           \n\
			.memcpy_bottom:         \n\
			testl   %%edx, %%edx    \n\
			jz      .memcpy_done    \n\
			movb    (%%esi), %%al   \n\
			movb    %%al, (%%edi)   \n\
			addl    $1, %%edi       \n\
			addl    $1, %%esi       \n\
			subl    $1, %%edx       \n\
			jmp     .memcpy_bottom  \n\
			.memcpy_done:           \n\
			"
			:
			: "S"(src), "D"(dest), "c"(n)
			: "eax", "edx", "memory", "cc"
			);

	return dest;
}

/*
* void* memmove(void* dest, const void* src, uint32_t n);
*   Inputs: void* dest = destination of move
*			const void* src = source of move
*			uint32_t n = number of byets to move
*   Return Value: pointer to dest
*	Function: move n bytes of src to dest
*/
/* Optimized memmove (used for overlapping memory areas) */
void* memmove(void* dest, const void* src, uint32_t n)
{
	asm volatile("                  \n\
			movw    %%ds, %%dx      \n\
			movw    %%dx, %%es      \n\
			cld                     \n\
			cmp     %%edi, %%esi    \n\
			jae     .memmove_go     \n\
			leal    -1(%%esi, %%ecx), %%esi    \n\
			leal    -1(%%edi, %%ecx), %%edi    \n\
			std                     \n\
			.memmove_go:            \n\
			rep     movsb           \n\
			"
			:
			: "D"(dest), "S"(src), "c"(n)
			: "edx", "memory", "cc"
			);

	return dest;
}

/*
* int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
*   Inputs: const int8_t* s1 = first string to compare
*			const int8_t* s2 = second string to compare
*			uint32_t n = number of bytes to compare
*	Return Value: A zero value indicates that the characters compared
*					in both strings form the same string.
*				A value greater than zero indicates that the first
*					character that does not match has a greater value
*					in str1 than in str2; And a value less than zero
*					indicates the opposite.
*	Function: compares string 1 and string 2 for equality
*/
int32_t strncmp(const int8_t* s1, const int8_t* s2, uint32_t n)
{
	int32_t i;
	for(i=0; i<n; i++) {
		if( (s1[i] != s2[i]) ||
				(s1[i] == '\0') /* || s2[i] == '\0' */ ) {

			/* The s2[i] == '\0' is unnecessary because of the short-circuit
			 * semantics of 'if' expressions in C.  If the first expression
			 * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
			 * s2[i], then we only need to test either s1[i] or s2[i] for
			 * '\0', since we know they are equal. */

			return s1[i] - s2[i];
		}
	}
	return 0;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*   Return Value: pointer to dest
*	Function: copy the source string into the destination string
*/
int8_t* strcpy(int8_t* dest, const int8_t* src)
{
	int32_t i=0;
	while(src[i] != '\0') {
		dest[i] = src[i];
		i++;
	}

	dest[i] = '\0';
	return dest;
}

/*
* int8_t* strcpy(int8_t* dest, const int8_t* src, uint32_t n)
*   Inputs: int8_t* dest = destination string of copy
*			const int8_t* src = source string of copy
*			uint32_t n = number of bytes to copy
*   Return Value: pointer to dest
*	Function: copy n bytes of the source string into the destination string
*/
int8_t* strncpy(int8_t* dest, const int8_t* src, uint32_t n)
{
	int32_t i=0;
	while(src[i] != '\0' && i < n) {
		dest[i] = src[i];
		i++;
	}

	while(i < n) {
		dest[i] = '\0';
		i++;
	}

	return dest;
}

/*
* void print_cr3(void)
* effects: prints the contents of the CR3 Register upon receiving an exception
*/
void print_cr3(void){
	uint32_t cr3;
	asm volatile(
                 "movl %%cr3, %%eax;"
				 "movl %%eax, %0;"
                 :"=r"(cr3)                      /* no outputs */
                 :						    /* no input */
                 :"%eax"                /* clobbered register */
                 );
	printf("\n                  Fault Occurs at: %x\n", cr3);
	return;
}

/*
* void test_interrupts(void)
*   Inputs: void
*   Return Value: void
*	Function: increments video memory. To be used to test rtc
*/
void test_interrupts(void)
{
	int32_t i;
	for (i=0; i < NUM_ROWS*NUM_COLS; i++) {
		video_mem[i<<1]++;
	}
	send_eoi(1);
}
