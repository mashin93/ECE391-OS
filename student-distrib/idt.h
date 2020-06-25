#ifndef IDT_H
#define IDT_H


/* local functions declared -- the different exceptions */
void stop(void);
void divide_error(void);
void debug(void);
void nmi(void);
void breakpoint(void);
void overflow(void);
void bounds(void);
void invalid_op(void);
void device_not_available(void);
void double_fault(void);
void segment_overrun(void);
void invalid_TSS(void);
void segment_not_present(void);
void stack_segment(void);
void general_protection(void);
void page_fault(void);
void generic_error(void);
void fp(void);
void alignment_check(void);
void machine_check(void);

/* setting up the intialization of the interrupt descriptor table */
void init_idt(void);

#endif // IDT
