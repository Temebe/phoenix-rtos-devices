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

#define LIN_ACC_SENSITIVITY 0.000122 /* Value of sensitivity for +-4g acceleration */
#define LSM303D_IDENTITY 0x49

#define WHO_AM_I    0x0F
#define CTRL1       0x20
#define CTRL2       0x21
#define OUT_X_L_A   0x28
#define OUT_X_H_A   0x29
#define OUT_Y_L_A   0x2A
#define OUT_Y_H_A   0x2B
#define OUT_Z_L_A   0x2C
#define OUT_Z_H_A   0x2D

#define WRITE_OP    0x00
#define READ_OP     0x80

static uint8_t readRegister(msg_t *msg, uint8_t address) 
{
    msg->type = mtRead;
    ((uint8_t *)msg->i.data)[0] = READ_OP | address;
    ((uint8_t *)msg->i.data)[1] = 0xff;

    if (msgSend(msg->i.io.oid.port, msg) < 0) 
        printf("imu: Error occured while trying to send readRegister.\n");

    return ((uint8_t *)msg->o.data)[1]; /* Only second byte is important to us */
}

static int writeRegister(msg_t *msg, uint8_t address, uint8_t message)
{
    msg->type = mtRead;
    ((uint8_t *)msg->i.data)[0] = WRITE_OP | address;
    ((uint8_t *)msg->i.data)[1] = message;

    if (msgSend(msg->i.io.oid.port, msg) < 0) {
        printf("imu: Error occured while trying to send writeRegister.\n");
        return -1;
    }
    
    return 0;
}

static void setupCommunication(msg_t *msg, oid_t *oid)
{
    struct spiserver_ctl ctl;

    while (lookup("/dev/spi3", NULL, oid) < 0)
        usleep(100000);

    ctl.fun = set_init | set_channel | set_clockDiv;
    ctl.oid = *oid;
    ctl.dev_no = ecspi4;
    ctl.chan_msk = 0x01;
    ctl.chan = 0;
    ctl.pre = 0x4;
    ctl.post = 0x4;

    msg->type = mtDevCtl;
    memcpy(msg->i.raw, &ctl, sizeof(ctl));
    msgSend(oid->port, msg);
}

int main(int argc, char **argv)
{
    msg_t msg = {0};
    oid_t oid;
    uint8_t data[2];
    uint8_t response[2];
    int16_t accX = 0, accY = 0, accZ = 0;

    printf("imu[%s %s]\n", __DATE__, __TIME__);
    setupCommunication(&msg, &oid);

    msg.type = mtRead;
    msg.i.io.oid = oid;
    msg.i.size = 2;
    msg.i.data = data;
    msg.o.size = 2;
    msg.o.data = response;

    writeRegister(&msg, CTRL1, 0x37);
    /* Set acceleration full-scale to +-4g */
    writeRegister(&msg, CTRL2, 0x08);
    
    if (readRegister(&msg, WHO_AM_I) != LSM303D_IDENTITY)
        printf("imu: Imu gave wrong answer to whoami question!\n");

    printf("\n");

    for(;;) {
        accX = readRegister(&msg, OUT_X_H_A) << 8 | readRegister(&msg, OUT_X_L_A);
        accY = readRegister(&msg, OUT_Y_H_A) << 8 | readRegister(&msg, OUT_Y_L_A);
        accZ = readRegister(&msg, OUT_Z_H_A) << 8 | readRegister(&msg, OUT_Z_L_A);

        printf("\rx: %6.3f\ty:%6.3f\tz:%6.3f ", accX * LIN_ACC_SENSITIVITY,
                                                accY * LIN_ACC_SENSITIVITY,
                                                accZ * LIN_ACC_SENSITIVITY);
        fflush(stdout);

        usleep(100000);
    }
}