/*-
 * Copyright (c) 2023, KusaReMKN.
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

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <jansson.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <curl/curl.h>

#include "compass.h"
#include "debug.h"
#include "dirdist.h"
#include "drive.h"
#include "gps.h"
#include "manki.h"

static void	calibration(void);
static void	countDown(int sec);

static void
calibration(void)
{
	int fd, ok;
	union calib val;

	fd = openCompass();
	if (fd == -1)
		err(1, "openCompass");
	ok = 0;
	do {
		if (readCalibStat(fd, &val) == -1)
			err(1, "getCalib");
		ok = val.sys == 3 ? ok + 1 : 0;
		DEBUG_printf("Calib: %d\r", val.sys);
	} while (ok < 3);
	DEBUG_printf("\n");
	(void)close(fd);

	return;
}

static void
countDown(int sec)
{
	do {
		DEBUG_printf("... %d          \r", sec);
		sleep(1);
	} while (--sec > 0);
	DEBUG_printf("\n");
}

int
main(int argc, char *argv[])
{
	int sock, step, sv[2];
	pthread_t driver;
	struct latlng cur;
	char q;
	struct carContext ctx = { 0 };
	CURLcode res;
	int next;

	(void)setbuf(stderr, NULL);

	calibration();
	DEBUG_printf("Calibration Complete!\n");
	fflush(stderr);
	countDown(20);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");
	if (initGps() == -1)
		err(1, "initGps");
	DEBUG_printf("Initialize Complete!\n");
	sleep(10);
	if (readGps(&cur) == -1)
		err(1, "getLocation");
	DEBUG_printf("Cur: %f, %f\n", cur.lat, cur.lng);
	res = manki(&ctx, MR_HELLO, &cur, 57);
	if (res != CURLE_OK)
		errx(1, "manki(HELLO): %d\n", res);

	sleep(3);

	if (pthread_create(&driver, NULL, drive, (void *)&sv[1]) != 0)
		err(1, "pthread_create");

	sock = sv[0];
	step = 0;
	while (/* CONSTCOND */ 1) {
		do {
			sleep(1);
			if (readGps(&cur) == -1)
				warn("readGps");
			else
				DEBUG_printf("Location: %.12f, %.12f\n",
						cur.lat, cur.lng);
			res = manki(&ctx, MR_NEXT, &cur, 57);
			if (res != CURLE_OK)
				errx(1, "manki(NEXT): %d\n", res);
			next = 0;
			if (strcmp(ctx.response, "next") == 0)
				next = 1;
			free(ctx.response);
		} while (!next);
		if (write(sock, &ctx.destination, sizeof(ctx.destination))
				== -1)
			err(1, "write");

		if (read(sock, &q, 1) == -1)
			err(1, "read");
		DEBUG_printf("driver says: %c\n", q);
	}
	/* NOTREACHED */

	return -1;      /* NEVER RETURNED */
}
