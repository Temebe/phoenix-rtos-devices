/*
 * Phoenix-RTOS
 * 
 * LSM303D imu reader
 * 
 * Copyright 2020 Phoenix Systems
 * Author: Sebastian Aksamit
 * 
 * This file is part of Phoenix-RTOS
 * 
 * %LICENSE%
 */

#include <sys/msg.h>
#include <unistd.h>
#include <stdio.h>

#include <spiserver.h>


int main(int argc, char **argv)
{
    printf("imu[%s %s]\n", __DATE__, __TIME__);
    msg_t msg = {0}; /* Is this {0} ok? */
    oid_t oid;
    struct spiserver_ctl ctl;
    int err;
    uint8_t data[2];
    uint8_t response[2];

    while (lookup("/dev/spi3", NULL, &oid) < 0)
        usleep(100000);

    //printf("imu: Looked up, port: %d, id: %lld\n", oid.port, oid.id);

    ctl.fun = set_init | set_channel | set_clockDiv; // | set_mode | set_clockDiv | set_csDelay | set_ssDelay;
    ctl.oid = oid;
    ctl.dev_no = ecspi4;
    ctl.chan_msk = 0x00;
    ctl.chan = 0; /* SPI communication in lsm303d */
    // ctl.mode = 2;
    ctl.pre = 0xA;
    ctl.post = 0xA;
    // ctl.csDelay = 10;
    // ctl.ssDelay = 500;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, &ctl, sizeof(ctl));
    msgSend(oid.port, &msg);
    printf("imu: About to enter the loop\n");

    // data[0] = ;
    // data[1] = ;

    // msg.type = mtRead;
    // msg.i.size = 2;
    // msg.i.data = data;
    // msg.o.size = 2;
    // msg.o.data = &response;

    for(;;) {
        data[0] = 0x88;
        data[1] = 0x00;

        msg.type = mtRead;
        //msg.i.size = data != NULL ? strlen(data) : 0;
        msg.i.size = 1;
        msg.i.data = data;
        msg.o.size = 2;
        msg.o.data = &response;

        printf("imu: Try to send %X, size %d\n", *data, msg.i.size);

        if (msgSend(oid.port, &msg) < 0)
            printf("imu: Unable to send message\n");
        else
            printf("imu: Received answer: %X and %X\n", *(uint8_t *)msg.o.data, *(uint8_t *)(msg.o.data) + 1);

        if (msg.o.io.err != err_ok)
            printf("imu: there was error on read\n");
        usleep(300000);
    }
}