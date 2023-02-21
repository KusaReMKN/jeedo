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

#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "errwarn.h"
#include "denkino.h"

#define DENKINO_PATH	"/dev/ttyS0"
#define DENKINO_MAX_LEN	72

int
closeDenkino(int fd, const struct termios *prev)
{
	if (prev != NULL)
		if (tcsetattr(fd, TCSANOW, prev) == -1)
			return -1;
	return close(fd);
}

int
openDenkino(struct termios *prev)
{
	int errsv, fd, flags;
	struct termios tio;

	/* Try to open the serial port */
	fd = open(DENKINO_PATH, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd == -1)
		return -1;
	/* Make it blocking mode */
	flags = fcntl(fd, F_GETFD);
	if (flags == -1) {
fatal:		errsv = errno;
		(void)close(fd);
		errno = errsv;
		return -1;
	}
	flags &= ~O_NONBLOCK;
	if (fcntl(fd, F_SETFD, flags) == -1)
		goto fatal;
	/* Get the previous attributes */
	if (prev != NULL)
		if (tcgetattr(fd, prev) == -1)
			goto fatal;
	/* Set the new attributes */
	memset(&tio, 0, sizeof(tio));
	tio.c_iflag = IGNBRK;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD;
	(void)cfsetspeed(&tio, B9600);
	tio.c_lflag = ICANON;
	if (tcsetattr(fd, TCSANOW, &tio) == -1)
		goto fatal;

	return fd;
}

int
talkDenkino(int fd, const char *restrict req, char *restrict res, size_t len)
{
	char msg[80], *p;
	int attempt, ret;
	fd_set rfds;
	struct timeval tv;

	/* Build the request message */
	p = strchr(req, '\n');
	if (p != NULL)
		*p = '\0';
	if (strlen(req) > DENKINO_MAX_LEN) {
		errno = EMSGSIZE;
		return -1;
	}
	if (snprintf(msg, sizeof(msg), "%s\n", req) < 0)
		return -1;
	/* Send the request message */
	attempt = 0;
	do {
		if (write(fd, msg, strlen(msg)) == -1)
			return -1;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		tv.tv_sec  = 1;
		tv.tv_usec = 0;
		ret = select(fd+1, &rfds, NULL, NULL, &tv);
		if (ret == -1)
			return -1;
		if (ret != 0)
			break;
		warning("Received no reply, retrying ... %d / 3\n", attempt);
	} while (attempt++ < 3);
	if (attempt > 3) {
		errno = EAGAIN;
		return -1;
	}
	/* Receive the response message */
	ret = read(fd, res, len-1);
	if (ret == -1)
		return -1;
	res[len-1] = '\0';
	p = strchr(res, '\n');
	if (p == NULL || ret > DENKINO_MAX_LEN) {
		errno = ENOBUFS;
		return -1;
	}
	*p = '\0';

	return 0;
}
