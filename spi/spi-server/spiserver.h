/*
 * Phoenix-RTOS
 * 
 * SPI communication manager
 * 
 * Copyright 2020 Phoenix Systems
 * Author: Sebastian Aksamit
 * `
 * This file is part of Phoenix-RTOS
 * 
 * %LICENSE%
 */

#include <sys/msg.h>
#include <ecspi.h> /* SPI api */

enum {
    set_init        = 0x00000001,
    set_channel     = 0x00000002,
    set_mode        = 0x00000004,
    set_clockDiv    = 0x00000008,
    set_csDelay     = 0x00000010,
    set_ssDelay     = 0x00000020
};

enum {
    err_ok = 0, /* everything went ok */
    err_init,   /* using not initialized spi */
    err_other   /* not described errors */
};

struct spiserver_ctl {
    oid_t oid; /* oid of spi port */
    int fun; /* Set function, should be a combination of spiserver_setFun enum flags */ 
    int dev_no;
    uint8_t chan_msk;
    uint8_t chan;
    uint8_t mode;
    uint8_t pre; /* pre-divider value for the clock signal */
    uint8_t post; /* post-divider value for the clock signal */
    uint8_t csDelay;
    uint16_t ssDelay;
};
