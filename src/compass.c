/*-
 * Copyright (c) 2022, KusaReMKN.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <errno.h>
#include <unistd.h>

#include "bno055.h"
#include "compass.h"
#include "i2c.h"

#define COMPASS_BUS	1
#define COMPASS_ADDR	BNO055_ADDR_ALT

static int	changeMode(int fd, int mode);
static int	isBNO055(int fd);
static int 	isChecked(int fd);

static int
changeMode(int fd, int mode)
{
	int mod;

	mod = i2cReadByteData(fd, OPR_MODE);
	if (mod == -1)
		return -1;
	mod = (mod & 0xF0) | (mode & 0x0F);
	if (i2cWriteByteData(fd, OPR_MODE, mod) == -1)
		return -1;
	/*
	 * Switching from CONFIGMODE to any mode requires >=7 ms,
	 * switching from any mode to CONFIGMODE requires >= 19 ms
	 */
	usleep(mode & 0x0F ? 10000L : 25000L);

	return 0;
}

static int
isBNO055(int fd)
{
	int id;

	/* The value of CHIP_ID is always 0xA0 */
	id = i2cReadByteData(fd, CHIP_ID);
	if (id == -1)
		return -1;

	return id == 0xA0;
}

static int
isChecked(int fd)
{
	int res;

	/* If all tests are passed, lower 4 bits of res are all 1 */
	res = i2cReadByteData(fd, ST_RESULT);
	if (res == -1)
		return -1;

	return (res & 0x0F) == 0x0F;
}

int
openCompass(void)
{
	int errsv, fd, res;

	/* Open the BNO055 module */
	fd = i2cOpen(COMPASS_BUS, COMPASS_ADDR);
	if (fd == -1)
		return -1;
	/* Verify if it is BNO055 */
	res = isBNO055(fd);
	if (res == -1) {
fatal:		errsv = errno;
		(void)close(fd);
		errno = errsv;
		return -1;
	}
	if (!res) {
error:		(void)close(fd);
		errno = ENXIO;
		return -1;
	}
	/* Check the result of Power On Self Test */
	res = isChecked(fd);
	if (res == -1)
		goto fatal;
	if (!res)
		goto error;
	/* OK, make it compass mode (0x09) */
	if (changeMode(fd, 0x09) == -1)
		goto fatal;

	return fd;
}

int
readCalibStat(int fd, union calib *val)
{
	int value;

	value = i2cReadByteData(fd, CALIB_STAT);
	if (value == -1)
		return -1;
	val->raw = value;

	return 0;
}

int
readCompass(int fd, double *deg)
{
	int value;

	value = i2cReadWordData(fd, EUL_Heading);
	if (value == -1)
		return -1;
	*deg = value / 16.0;

	return 0;
}
