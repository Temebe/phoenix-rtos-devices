/*
 * Phoenix-RTOS
 *
 * STM32L4 flash driver.
 *
 * Copyright 2017, 2018 Phoenix Systems
 * Author: Jakub Sejdak, Aleksander Kaminski
 *
 * This file is part of Phoenix-RTOS.
 *
 * %LICENSE%
 */

#ifndef _FLASH_H_
#define _FLASH_H_

#include <stdint.h>
#include "common.h"
#include "config.h"

#define FLASH_PAGE_SIZE         2048
#define FLASH_OB_1_ADDR         0x1fff7800
#define FLASH_OB_2_ADDR         0x1ffff800
#define FLASH_OB_SIZE           16


static inline int flash_activeBank(void)
{
	uint32_t pc = getPC();

	if (pc < FLASH_PROGRAM_1_ADDR)
		pc += FLASH_PROGRAM_1_ADDR;

	return !(pc < FLASH_PROGRAM_2_ADDR);
}


extern size_t flash_readData(uint32_t offset, char *buff, size_t size);


extern size_t flash_writeData(uint32_t offset, const char *buff, size_t size);


extern int flash_init(void);


#endif
