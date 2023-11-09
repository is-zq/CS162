Crash reason:
    Read kernel virtual memory space in user program. So page fault occured.

Solution:
    Turn line *esp = PHYS_BASE; in setup_stack(void** esp) of process.c into *esp = ((uint8_t*)PHYS_BASE) - (offset);
    offset is an integer greater or equal than 0x0c and cannot be too large.
    The minimum value is 0x0c, possibly because the parameter is'/do-nothing\0'.
    In order to align the stack, we set it to 0x14.
