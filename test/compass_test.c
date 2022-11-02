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

#include <err.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#include "compass.h"

static jmp_buf	env;

static void
handler(int sig)
{
	longjmp(env, 1);
}

int
main(int argc, char *argv[])
{
	int cnt, fd;
	union calib val;
	double deg;

	fd = openCompass();
	if (fd == -1)
		err(1, "openCompass");

	if (setjmp(env) != 0)
		goto quit;
	(void)signal(SIGINT, handler);
	printf("Press [Ctrl]+[C] to terminate.\n");

	cnt = 0;
	do {
		usleep(100 * 1000);
		if (readCalibStat(fd, &val) == -1)
			err(1, "readCalibStat");
		cnt = val.sys == 3 ? cnt + 1 : 0;
		printf("Calibrating ... %d\r", val.sys);
		fflush(stdout);
	} while (cnt < 10);
	printf("\n");

	while (/* CONSTCOND */ 1) {
		usleep(100 * 1000);
		if (readCompass(fd, &deg) == -1)
			err(1, "readCompass");
		printf("Heading: %f\r", deg);
		fflush(stdout);
	}
quit:
	printf("\n");
	(void)close(fd);

	return 0;
}
