#ifndef IO_H_
# define IO_H_

# include <k/types.h>

# define PORT_COM1  0x3F8
# define PORT_COM2  0x2F8
# define PORT_COM3  0x3E8
# define PORT_COM4  0x2E8

# define PORT_PIC_MASTER_CMD    0x20
# define PORT_PIC_MASTER_DATA   0x21
# define PORT_PIC_SLAVE_CMD     0xA0
# define PORT_PIC_SLAVE_DATA    0xA1

# define PORT_PS2_DATA      0x60
# define PORT_PS2_STATUS    0x64
# define PORT_PS2_CMD       0x64

static inline void outb(u16 port, u8 val)
{
    __asm__ volatile ("outb %[val], %[port]\n"
                      :: [val]"a"(val), [port]"d"(port));
}

static inline u8 inb(u16 port)
{
    u8 val;
    __asm__ volatile ("inb  %[port], %[val]\n"
                      : [val]"=a"(val)
                      : [port]"d"(port));
    return val;
}

#endif				/* !IO_H_ */
