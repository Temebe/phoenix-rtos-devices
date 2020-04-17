# spiserver

Server managing communication between software and various devices through SPI interface using imx6ull-ecspi library.

# Usage

First thing that needs to be done is finding spiserver's `oid`. It can be found with `lookup` function at `/dev/spiX`, where X is {0, 1, 2, 3}. X is connected to ecspi device number, where `spi0` stands for `ecspi1`, `spi1` for `ecspi2` etc.

## Configuration - mtDevCtl

Spiserver uses imx6ull-ecspi library, therefore interface for configuration is tightly connected to it. It is done by sending message of type `mtDevCtl` and with it's raw input filled with spiserver_ctl structure.

```
enum {
    set_init        = 0x00000001,
    set_channel     = 0x00000002,
    set_mode        = 0x00000004,
    set_clockDiv    = 0x00000008,
    set_csDelay     = 0x00000010,
    set_ssDelay     = 0x00000020
};

struct spiserver_ctl {
    oid_t oid;
    int fun;
    int dev_no;
    uint8_t chan_msk;
    uint8_t chan;
    uint8_t mode;
    uint8_t pre;
    uint8_t post;
    uint8_t csDelay;
    uint16_t ssDelay;
};
```

The idea is that with one message there can be done multiple settings at once by combining flags in `fun` field. For example initializing interface and setting channel may be done by filling `fun` with `set_init | set_channel` value. All flags and fields in structure are a reflection of ecspi library functions and are in greater detail described [here](https://github.com/phoenix-rtos/phoenix-rtos-devices/tree/master/spi/imx6ull-ecspi).

Example

```
    struct spiserver_ctl ctl;

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
```

## Synchronous data exchange - mtRead

Sending message with intention to capture response can be done by sending message of `mtRead` type. It is needed to provide buffor with outgoing and for incoming data. These can be pointing to the same memory without risk of race. Also .io.oid field has to be field to identify which spi device is addressed.

Example
```
    msg_t msg;
    oid_t oid;
    uint_t data[3];
    uint_t response[3];

    /* ... */

    msg.type = mtRead;
    msg.i.io.oid = oid;
    msg.i.size = 3;
    msg.i.data = data;
    msg.o.size = 3;
    msg.o.data = response;

    msgSend(oid, msg)
```

## Writing without reading - mtWrite

If response is not needed message of `mtWrite` can be used instead of `mtRead`. It uses asynchronous version to just send message through and does nothing more. Usage is same as for `mtRead` with the exception for message type of course.
