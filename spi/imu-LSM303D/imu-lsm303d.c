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
    msg_t msg = {0};
    oid_t oid;
    struct spiserver_ctl ctl;
    int err;
    uint8_t data[2];
    uint8_t response[2];
    int16_t ax = 0;

    while (lookup("/dev/spi3", NULL, &oid) < 0)
        usleep(100000);

    ctl.fun = set_init | set_channel | set_clockDiv;
    ctl.oid = oid;
    ctl.dev_no = ecspi4;
    ctl.chan_msk = 0x01;
    ctl.chan = 0;
    ctl.pre = 0x4;
    ctl.post = 0x4;

    msg.type = mtDevCtl;
    memcpy(msg.i.raw, &ctl, sizeof(ctl));
    msgSend(oid.port, &msg);

    data[0] = 0x20;
    data[1] = 0x37;

    msg.type = mtRead;
    msg.i.size = 2;
    msg.i.data = data;
    msg.o.size = 2;
    msg.o.data = response;

    if (msgSend(oid.port, &msg) < 0)
        printf("imu: Unable to send message\n");
    
    data[0] = 0x8F;
    data[1] = 0x8F;

    /* To put brackets or not to put */
    if (msgSend(oid.port, &msg) < 0)
        printf("imu: Unable to send message\n");
    else {
        if (response[1] != 0x49) {
            printf("imu: Imu gave wrong answer to whoami question!\n");
        }
    }

    printf("\n");

    for(;;) {
        data[0] = 0xA9;
        data[1] = 0xA9;

        if (msgSend(oid.port, &msg) < 0)
            printf("imu: Unable to send message\n");
        else
            ax = response[1] << 8;

        data[0] = 0xA8;
        if (msgSend(oid.port, &msg) < 0)
            printf("imu: Unable to send message\n");
        else
            ax |= response[1];

        printf("\rax: %d        ", ax);
        fflush(stdout);

        if (msg.o.io.err != err_ok)
            printf("imu: there was error on read\n");
        usleep(100000);
    }
}