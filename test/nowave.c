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

#include <sys/types.h>
#include <sys/socket.h>

#include <err.h>
#include <jansson.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "compass.h"
#include "debug.h"
#include "dirdist.h"
#include "drive.h"
#include "gps.h"

static void		calibration(void);
static void		countDown(int sec);
static struct latlng *	readRoute(const char *path);

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
	struct latlng *points;
	struct latlng cur;
	char q;

	(void)setbuf(stderr, NULL);

	points = readRoute("route.json");
	if (points == NULL)
		err(1, "readRoute");

	calibration();
	DEBUG_printf("Calibration Complete!\n");
	countDown(10);

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1)
		err(1, "socketpair");
	if (initGps() == -1)
		err(1, "initGps");
	DEBUG_printf("Initialize Complete!\n");

	if (pthread_create(&driver, NULL, drive, (void *)&sv[1]) != 0)
		err(1, "pthread_create");

	sock = sv[0];
	step = 0;
	while (/* CONSTCOND */ 1) {
		if (write(sock, &points[step++], sizeof(points[0])) == -1)
			err(1, "write");

		if (isnan(points[step].lat))
			step = 0;

		if (read(sock, &q, 1) == -1)
			err(1, "read");
		DEBUG_printf("driver says: %c\n", q);
		if (readGps(&cur) == -1)
			warn("readGps");
		else
			DEBUG_printf("Location: %.12f, %.12f\n",
					cur.lat, cur.lng);
	}
	/* NOTREACHED */

	return -1;	/* NEVER RETURNED */
}

static struct latlng *
readRoute(const char *path)
{
	json_t *elem, *lat, *lng, *root;
	size_t len, z;
	struct latlng *p;

	root = json_load_file(path, 0, NULL);
	if (root == NULL)
		return NULL;

	len = json_array_size(root);
	if (len == 0) {
fatal:		json_decref(root);
		return NULL;
	}

	p = malloc(sizeof(*p) * (len+1));
	if (p == NULL)
		goto fatal;

	for (z = 0; z < len; z++) {
		elem = json_array_get(root, z);
		if (elem == NULL)
			goto fatal;

		lat = json_object_get(elem, "lat");
		if (lat == NULL) {
			json_decref(elem);
			goto fatal;
		}
		lng = json_object_get(elem, "lng");
		if (lng == NULL) {
			json_decref(lat);
			json_decref(elem);
			goto fatal;
		}
		p[z].lat = json_number_value(lat);
		p[z].lng = json_number_value(lng);

		json_decref(lng);
		json_decref(lat);
		json_decref(elem);
	}
	p[z].lat = p[z].lng = NAN;
	json_decref(root);

	return p;
}
