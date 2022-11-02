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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/types.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>

#include "i2c.h"

#define I2C_PATH_FMT	"/dev/i2c-%d"

static int	i2cAccess(int fd, int rdwr, int reg, int size,
						union i2c_smbus_data *data);

static int
i2cAccess(int fd, int rdwr, int reg, int size, union i2c_smbus_data *data)
{
	struct i2c_smbus_ioctl_data arg;

	arg.read_write = rdwr;
	arg.command    = reg;
	arg.size       = size;
	arg.data       = data;

	return ioctl(fd, I2C_SMBUS, &arg);
}

int
i2cOpen(int bus, int addr)
{
	int errsv, fd, written;
	char path[PATH_MAX];

	/* Build the file name of device */
	written = snprintf(path, sizeof(path), I2C_PATH_FMT, bus);
	if (written < 0)
		return -1;
	if (written >= sizeof(path)) {
		errno = ENAMETOOLONG;
		return -1;
	}

	/* Open the device file */
	fd = open(path, O_RDWR);
	if (fd == -1)
		return -1;
	/* specify the address */
	if (ioctl(fd, I2C_SLAVE, (long)addr) == -1) {
		errsv = errno;
		(void)close(fd);
		errno = errsv;
		return -1;
	}

	return fd;
}

int
i2cReadByte(int fd)
{
	union i2c_smbus_data data;

	if (i2cAccess(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data) == -1)
		return -1;

	return (int)data.byte;
}

int
i2cReadByteData(int fd, int reg)
{
	union i2c_smbus_data data;

	if (i2cAccess(fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA, &data)
			== -1)
		return -1;

	return data.byte;
}

int
i2cReadWordData(int fd, int reg)
{
	union i2c_smbus_data data;

	if (i2cAccess(fd, I2C_SMBUS_READ, reg, I2C_SMBUS_WORD_DATA, &data)
			== -1)
		return -1;

	return data.word;
}

int
i2cWriteByte(int fd, int reg)
{
	return i2cAccess(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE, NULL);
}

int
i2cWriteByteData(int fd, int reg, int byte)
{
	union i2c_smbus_data data;

	data.byte = (__u8)byte;

	return i2cAccess(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, &data);
}

int
i2cWriteQuick(int fd, int rdwr)
{
	return i2cAccess(fd, rdwr, 0, I2C_SMBUS_QUICK, NULL);
}

int
i2cWriteWordData(int fd, int reg, int word)
{
	union i2c_smbus_data data;

	data.word = (__u16)word;

	return i2cAccess(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_WORD_DATA, &data);
}
