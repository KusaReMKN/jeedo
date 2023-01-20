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
#include <math.h>
#include <string.h>
#include <unistd.h>

#include "compass.h"
#include "debug.h"	/* !!! */
#include "denkino.h"
#include "dirdist.h"
#include "drive.h"
#include "gps.h"
#include "lidar.h"

static void	cancelNoise(struct lidarPoint lps[360]);
static int	detectObstacle(const struct lidarPoint lps[360]);
static int	getLocation(struct latlng *cur);
static int	getNextDest(int sock, struct latlng *dest);

static void
cancelNoise(struct lidarPoint lps[360])
{
#define INTENSITY_THRESHOLD	64
#define DISTANCE_TOO_FAR	12000	/* [mm] */
#define DISTANCE_TOO_CLOSE	300	/* [mm]; FIXME */
	int i;

	for (i = 0; i < 360; i++) {
		/* Low intensity points can be ignored. */
		if (lps[i].intensity <= INTENSITY_THRESHOLD)
			lps[i].distance = DISTANCE_TOO_FAR;
		/* Points too far or too close can be ignored. */
		if (lps[i].distance > DISTANCE_TOO_FAR
				|| lps[i].distance < DISTANCE_TOO_CLOSE)
			lps[i].distance = DISTANCE_TOO_FAR;
	}
}

static int
detectObstacle(const struct lidarPoint lps[360])
{
#define ANGLE_OF_VIEW		90	/* +-45 [deg]; FIXME */
#define DISTANCE_TO_OBSTACLE	3000	/* [mm] */
	int i;

	for (i = 360 - ANGLE_OF_VIEW / 2; i < 360; i++)
		if (lps[i].distance <= DISTANCE_TO_OBSTACLE)
			return 1;
	for (i = 0; i < ANGLE_OF_VIEW / 2; i++)
		if (lps[i].distance <= DISTANCE_TO_OBSTACLE)
			return 1;

	return 0;
}

void *
drive(void *arg)
{
	int compass, denkino, sock;
	struct latlng cur, dest;
	double angle, heading, leftAngle, rightAngle;
	char errmesg[256], *reqmesg, resmesg[80];
	struct lidarPoint lps[360];

	/* Receive the first destination */
	sock = *(int *)arg;
	if (read(sock, &dest, sizeof(dest)) == -1)
		return (void *)-1;
	/* Initialize the peripheral devices */
	compass = openCompass();
	if (compass == -1)
		return (void *)-1;
	denkino = openDenkino(NULL);
	if (denkino == -1)
		goto error;
	if (initLidar() == -1) {
		(void)close(denkino);
error:		(void)close(compass);
		return (void *)-1;
	}
	/* Main loop */
	while (/* CONSTCOND */ 1) {
		if (getLocation(&cur) == -1)
			goto halt;
		DEBUG_printf("cur -> dest : (%.9f, %.9f) -> (%.9f, %.9f)\n",
				cur.lat, cur.lng, dest.lat, dest.lng);
		if (isReached(&cur, &dest)) {
			if (talkDenkino(denkino, "B", resmesg, sizeof(resmesg))
					== -1)
				goto halt;
			DEBUG_printf("Reached!!\n");
			sleep(1);
			if (getNextDest(sock, &dest) == -1)
				goto halt;
			continue;
		}
		if (readLidar(lps) == -1)
			goto halt;
		cancelNoise(lps);
		if (detectObstacle(lps)) {
			if (talkDenkino(denkino, "B", resmesg, sizeof(resmesg))
					== -1)
				goto halt;
			usleep(100000);
			continue;
		}
		angle = direction(&cur, &dest);
		if (readCompass(compass, &heading) == -1)
			goto halt;
		DEBUG_printf("heading -> angle : %f -> %f\n", heading, angle);
		if (isDirected(heading, angle)) {
			DEBUG_printf("Directed!!\n");
			if (talkDenkino(denkino, "F", resmesg, sizeof(resmesg))
					== -1)
				goto halt;
			usleep(100000);
			continue;
		}
		leftAngle  = normalize(heading - angle);
		rightAngle = normalize(angle - heading);
		DEBUG_printf("L, R : %f, %f\n", leftAngle, rightAngle);
		if (isOutside(&cur, heading, &dest))
			reqmesg = leftAngle < rightAngle ? "FL" : "FR";
		else
			reqmesg = leftAngle < rightAngle ? "L" : "R";
		if (talkDenkino(denkino, reqmesg, resmesg,
					sizeof(resmesg)) == -1)
			goto halt;
		usleep(50000);
	}
halt:	/* Handle all errors */
	(void)strerror_r(errno, errmesg, sizeof(errmesg));
	DEBUG_printf("Last Error: %s\n", errmesg);
	DEBUG_printf("Damn it!  Halting ...\n");
	while (/* CONSTCOND */ 1) {
		(void)talkDenkino(denkino, "H", resmesg, sizeof(resmesg));
		(void)write(sock, "H", 1);
		DEBUG_printf("Halted!!\n");
		(void)sleep(1);
	}
	/* NOTREACHED */
}

static int
getLocation(struct latlng *cur)
{
	do {
		if (readGps(cur) == -1)
			return -1;
	} while (isnan(cur->lat) || isnan(cur->lng));

	return 0;
}

static int
getNextDest(int sock, struct latlng *dest)
{
	if (write(sock, "R", 1) == -1)
		return -1;
	if (read(sock, dest, sizeof(*dest)) == -1)
		return -1;

	return 0;
}
