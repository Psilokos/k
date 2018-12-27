#include <stdio.h>
#include "cqueue.h"
#include "io.h"
#include "ps2.h"
#include "timer.h"

#define PS2_STATUS_WAIT_OUTPUT  1
#define PS2_STATUS_WAIT_INPUT   2

#define PS2_CMD_PORT0_DISABLE   0xAD
#define PS2_CMD_PORT0_ENABLE    0xAE
#define PS2_CMD_PORT1_DISABLE   0xA7
#define PS2_CMD_PORT1_ENABLE    0xA8

#define PS2_CMD_TEST_SELF   0xAA
#define PS2_CMD_TEST_PORT0  0xAB
#define PS2_CMD_TEST_PORT1  0xA9

#define PS2_SELFTEST_SUCCESS    0x55

#define PS2_CMD_PULSE_LINES 0xF0

#define PS2_KBD_CMD_SCANCODE_SET    0xF0

#define PS2_KBD_CMD_RESET           0xFF
#define PS2_KBD_SELFTEST_SUCCESS    0xAA

#define PS2_KBD_ACK         0xFA
#define PS2_KBD_RESEND_CMD  0xFE

static inline int
ps2_wait(u8 status_flag, int clear)
{
    u32 timeout_time = timer_get_time() + 10;
    while (!!(inb(PORT_PS2_STATUS) & status_flag) == clear)
        if (timer_get_time() >= timeout_time)
            return -1;
    return 0;
}

static inline int
ps2_read(u8 *p_data)
{
    if (ps2_wait(PS2_STATUS_WAIT_OUTPUT, 0))
        return -1;
    *p_data = inb(PORT_PS2_DATA);
    return 0;
}

static inline int
ps2_write(u8 data)
{
    if (ps2_wait(PS2_STATUS_WAIT_INPUT, 1))
        return -1;
    outb(PORT_PS2_DATA, data);
    return 0;
}

static inline int
ps2_send_cmd(u8 cmd)
{
    // FIXME wait?
    outb(PORT_PS2_CMD, cmd);
    return 0;
}

static inline int
ps2_kbd_send_cmd(u8 cmd)
{
    u8 resp;
    int i = 0;
send_cmd:
    if (ps2_write(cmd)) return -1;
    if (ps2_read(&resp)) return -1;
    if (resp == PS2_KBD_RESEND_CMD && i++ < 3)
        goto send_cmd;
    return resp != PS2_KBD_ACK;
}

#define PS2_CMD_CFG_READ    0x20
#define PS2_CMD_CFG_WRITE   0x60

static inline int
ps2_cfg_read(u8 *p_cfg)
{
    if (ps2_send_cmd(PS2_CMD_CFG_READ)) return -1;
    if (ps2_read(p_cfg)) return -1;
    return 0;
}

static inline int
ps2_cfg_write(u8 cfg)
{
    if (ps2_send_cmd(PS2_CMD_CFG_WRITE)) return -1;
    if (ps2_write(cfg)) return -1;
    return 0;
}

enum
{
    PS2_CFG_PORT0_IRQ       = 1 << 0,
    PS2_CFG_PORT1_IRQ       = 1 << 1,
    PS2_CFG_POST            = 1 << 2,
    PS2_CFG_PORT0_NOCLOCK   = 1 << 4,
    PS2_CFG_PORT1_NOCLOCK   = 1 << 5,
    PS2_CFG_TRANSLATION     = 1 << 6
};

static inline int
ps2_set_cfg(u8 *p_cfg)
{
    if (ps2_cfg_read(p_cfg)) return -1;
    *p_cfg &=
        ~PS2_CFG_PORT0_IRQ &
        ~PS2_CFG_PORT1_IRQ &
        ~PS2_CFG_TRANSLATION;
    if (ps2_cfg_write(*p_cfg)) return -1;
    return 0;
}

static int
ps2_self_test(u8 cfg)
{
    u8 test;
    if (ps2_send_cmd(PS2_CMD_TEST_SELF)) return -1;
    if (ps2_read(&test)) return -1;
    int ret = test == PS2_SELFTEST_SUCCESS ? 0 : -1;
    if (!ret)
        if (ps2_cfg_write(cfg)) return -1;
    return ret;
}

static int
ps2_detect_dualchan(u8 cfg, u8 *dualchan)
{
    if (cfg & PS2_CFG_PORT1_NOCLOCK)
    {
        if (ps2_send_cmd(PS2_CMD_PORT1_ENABLE)) return -1;
        if (ps2_cfg_read(&cfg)) return -1;
        if (!(cfg & PS2_CFG_PORT1_NOCLOCK))
        {
            if (ps2_send_cmd(PS2_CMD_PORT1_DISABLE)) return -1;
            *dualchan = 1;
            return 0;
        }
    }
    *dualchan = 0;
    return 0;
}

static int
ps2_test_ports(u8 *avlports)
{
    u8 test[2];
    if (ps2_send_cmd(PS2_CMD_TEST_PORT0)) return -1;
    if (ps2_read(test + 0)) return -1;
    if (*avlports & (1 << 1))
    {
        if (ps2_send_cmd(PS2_CMD_TEST_PORT1)) return -1;
        if (ps2_read(test + 1)) return -1;
    }
    *avlports &= (!test[0] << 0) | (!test[1] << 1);
    return 0;
}

static int ps2_kbd_init(void);

int
ps2_init(void)
{
    /* TODO check for ps2 controller presence */

    if (ps2_send_cmd(PS2_CMD_PORT0_DISABLE)) return -1;
    if (ps2_send_cmd(PS2_CMD_PORT1_DISABLE)) return -1;
    inb(PORT_PS2_DATA); /* flush data port */

    u8 cfg;
    if (ps2_set_cfg(&cfg)) return -1;
    if (ps2_self_test(cfg)) return -1;

    u8 avlports;
    if (ps2_detect_dualchan(cfg, &avlports)) return -1;
    avlports = (avlports << 1) | 1;
    if (ps2_test_ports(&avlports)) return -1;

    if (!avlports)
    {
        // FIXME use VGA
        for (int i = 0; i < 5; ++i)
        {
            printf("PS2 error: no available port. Aborting... %i\n", 5 - i);
            timer_sleep(10);
        }
        ps2_send_cmd(PS2_CMD_PULSE_LINES);
    }

    if (avlports & (1 << 0))
        if (ps2_kbd_init()) return -1;
    if (avlports & (1 << 1))
        if (ps2_send_cmd(PS2_CMD_PORT1_ENABLE)) return -1;

    return 0;
}

/* ******** *
 * keyboard *
 * ******** */

static cqueue_def(u8, 256) keycode_q;

static int
ps2_kbd_init(void)
{
    u8 selftest;
    if (ps2_kbd_send_cmd(PS2_KBD_CMD_RESET)) return -1;
    if (ps2_read(&selftest) ||
        selftest != PS2_KBD_SELFTEST_SUCCESS) return -1;

    if (ps2_kbd_send_cmd(PS2_KBD_CMD_SCANCODE_SET)) return -1;
    if (ps2_kbd_send_cmd(2)) return -1;

    cqueue_init(keycode_q);

    u8 cfg;
    if (ps2_cfg_read(&cfg)) return -1;
    if (ps2_cfg_write(cfg | PS2_CFG_PORT0_IRQ)) return -1;
    if (ps2_send_cmd(PS2_CMD_PORT0_ENABLE)) return -1;

    return 0;
}

int
ps2_kbd_save_keycode(u8 keycode)
{
    if (!cqueue_can_write(keycode_q))
        /* TODO bip */
        return -1;
    cqueue_write(keycode_q, keycode);
    return 0;
}

u8
ps2_kbd_has_key(void)
{
    if (!cqueue_can_read())
        return 0;
    u8 keycode = cqueue_read();
    u8 ret = keycode != 0xF0 || cqueue_can_read();
    cqueue_write_back(keycode);
    return ret;
}

u16
ps2_kbd_get_key(void)
{
}
