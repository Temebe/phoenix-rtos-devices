#
# Makefile for Phoenix-RTOS spi manager
#
# Copyright 2020 Phoenix Systems
#

$(PREFIX_PROG)spiserver: $(PREFIX_O)spi/spi-server/spiserver.o $(PREFIX_A)libecspi.a
	$(LINK)

$(PREFIX_H)spiserver.h: spi/spi-server/spiserver.h
	$(HEADER)

all: $(PREFIX_PROG_STRIPPED)spiserver $(PREFIX_H)spiserver.h
