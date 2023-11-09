Crash reason:
    Read kernel virtual memory space in user program. So page fault occured.

Solution:
    Turn line *esp = PHYS_BASE; in setup_stack(void** esp) of process.c into *esp = ((uint8_t*)PHYS_BASE) - (0x14);
    offset is an integer greater than 0x0c and cannot be too large
    The minimum value of 0x0c may be due to the three parameters of the main function
    In order for stack align 0 to pass, we need to set it to 0x14
