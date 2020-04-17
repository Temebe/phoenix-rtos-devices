/*
 * Phoenix-RTOS
 * 
 * SPI communication manager
 * 
 * Copyright 2020 Phoenix Systems
 * Author: Sebastian Aksamit
 * 
 * This file is part of Phoenix-RTOS
 * 
 * %LICENSE%
 */

#include <unistd.h> /* For usleep, delete later */
#include <stdio.h>
#include <errno.h>
#include <sys/threads.h>

#include "spiserver.h"

static const int thrPriority = 4;
static const int thrAmount = 3; /* Amount of additional threads */

/* Struct keeping */
struct spiserver_dev {
    int dev_no;
    ecspi_ctx_t ctx;
    int init; /* 0 -> ecspi not initialized, 1 -> ecspi initialized */
    handle_t cond; /* condition variable for asynchronous context */
    handle_t lock;
};

struct spiserver_threadData {
    unsigned int port;
    struct spiserver_dev *devs; /* Array of device structures */
};

static void spiserver_handleSynchronous(struct spiserver_dev *spi, msg_t *msg)
{
    mutexLock(spi->lock);

    if (spi->init != 1) {
        printf("spi: SPI not initialized\n");
        msg->o.io.err = err_init;
        mutexUnlock(spi->lock);
        return;
    }

    if (msg->i.data == NULL || msg->o.data == NULL) {
        printf("spi: Data null\n");
        msg->o.io.err = err_other;
        mutexUnlock(spi->lock);
        return;
    }

    /* According to docs size of buffer for incoming data has to be at least as big as outgoing data */
    if (msg->o.size < msg->i.size) {
        printf("spi: Input size smaller than output size (in: %d, o: %d\n", msg->i.size, msg->o.size);
        msg->o.io.err = err_other;
        mutexUnlock(spi->lock);
        return;
    }

    msg->o.io.err = err_ok;
    ecspi_exchange(spi->dev_no, msg->i.data, msg->o.data, msg->i.size);
    mutexUnlock(spi->lock);
}

static void spiserver_handleAsynchronous(struct spiserver_dev *spi, msg_t *msg)
{
    mutexLock(spi->lock);
    msg->o.io.err = err_ok;
    ecspi_writeAsync(&spi->ctx, msg->i.data, msg->i.size);
    mutexUnlock(spi->lock);
}

/* TODO remove unnecessary printf */
static void spiserver_handleCtl(struct spiserver_dev *spi, msg_t *msg)
{ 
    mutexLock(spi->lock);
    struct spiserver_ctl ctl = *(struct spiserver_ctl *)msg->i.raw;
    if ((ctl.fun & set_init) != 0) {
        ecspi_init(ctl.dev_no, ctl.chan_msk);
        printf("set_init %d %d\n", ctl.dev_no, ctl.chan_msk);
        spi->init = 1;
    }

    if (spi->init != 1) {
        *(int *)msg->o.raw = err_init;
        mutexUnlock(spi->lock);
        return;
    }

    if ((ctl.fun & set_channel) != 0) {
        ecspi_setChannel(ctl.dev_no, ctl.chan);
        printf("ecspi_setChannel %d %d\n", ctl.dev_no, ctl.chan);
    }

    if ((ctl.fun & set_mode) != 0) {
        ecspi_setMode(ctl.dev_no, ctl.chan, ctl.mode);
        printf("ecspi_setMode %d %d %d\n", ctl.dev_no, ctl.chan, ctl.mode);
    }

    if ((ctl.fun & set_clockDiv) != 0) {
        ecspi_setClockDiv(ctl.dev_no, ctl.pre, ctl.post);
        printf("ecspi_setClockDiv %d %d %d\n", ctl.dev_no, ctl.pre, ctl.post);
    }

    if ((ctl.fun & set_csDelay) != 0) {
        ecspi_setCSDelay(ctl.dev_no, ctl.csDelay);
        printf("ecspi_setCSDelay %d %d\n", ctl.dev_no, ctl.csDelay);
    }

    if ((ctl.fun & set_ssDelay) != 0) {
        ecspi_setSSDelay(ctl.dev_no, ctl.ssDelay);
        printf("ecspi_setSSDelay %d %d\n", ctl.dev_no, ctl.ssDelay);
    }
    
    mutexUnlock(spi->lock); 
}

/* 
 * Getting oid from message
 * Is there better way than based on message type?
 * This approach generates some code duplicates
 * 
 * Better name for args?
 */
static void spiserver_messageThread(void *arg)
{
    msg_t msg;
    struct spiserver_threadData *data;
    struct spiserver_dev *currentSpi = NULL;
    oid_t currentOid;
    unsigned int rid = 0;

    data = (struct spiserver_threadData*)arg;
    
    for (;;) {
        if (msgRecv(data->port, &msg, &rid) < 0)
            continue;

        switch (msg.type) {
        case mtDevCtl:
            currentOid = ((struct spiserver_ctl *)(msg.i.raw))->oid;
            currentSpi = &data->devs[currentOid.id];
            spiserver_handleCtl(currentSpi, &msg);
            break;

        case mtRead:
            currentOid = msg.i.io.oid;
            currentSpi = &data->devs[currentOid.id];
            spiserver_handleSynchronous(currentSpi, &msg);
            break;

        case mtWrite:
            currentOid = msg.i.io.oid;
            currentSpi = &data->devs[currentOid.id];
            spiserver_handleAsynchronous(currentSpi, &msg);
            break;

        default:
            printf("Unknown message\n");
            break;
        }

        msgRespond(data->port, &msg, rid);
    }
}

/* TODO Remove unnecessary printf later */
int main(int argc, char **argv)
{
    char stack[thrAmount][1024] __attribute__((aligned(8)));
    oid_t spiOid[4];
    struct spiserver_dev spi[4];
    struct spiserver_threadData data;
    char portName[10];

    printf("spi[%s %s]\n", __DATE__, __TIME__);

    /* Create and register port for clients */
    portCreate(&data.port);
    for (unsigned int i = 0; i < 4; ++i) {
        sprintf(&portName, "/dev/spi%d", i);
        spiOid[i].port = data.port;
        spiOid[i].id = i;
        if (portRegister(data.port, &portName, &spiOid[i]) < 0)
            printf("spi: Failed to create port %s\n", &portName);
    }

    /* Initialize spi device structures */
    spi[0].dev_no = ecspi1;
    spi[1].dev_no = ecspi2;
    spi[2].dev_no = ecspi3;
    spi[3].dev_no = ecspi4;

    for (unsigned int i = 0; i < 4; ++i) {
        spi[i].init = 0;
        condCreate(&spi[i].cond);
        mutexCreate(&spi[i].lock);
        ecspi_registerContext(spi[i].dev_no, &spi[i].ctx, spi[i].cond);
    }

    data.devs = &spi;
    /* Breaking lines? */
    for (unsigned int i = 0; i < thrAmount; ++i)
        beginthreadex(spiserver_messageThread, thrPriority, stack[i], sizeof(stack[i]), &data, NULL);
    spiserver_messageThread(&data);
    printf("spiserver: Stopped SPI server\n");

}