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
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "lidar.h"
#include "tekgraph.h"

#define deg2rad(x)	((x) * atan(1) / 45.0)
#define zoom(x)		((x) * 0.03)

static jmp_buf	env;

static void	drawBackground(void);
static void	drawLidar(struct lidarPoint points[360]);
static void	handler(int sig);

static void
drawBackground(void)
{
	beginLine(4);
	moveTo(511, 0);
	moveTo(511, 778);
	commit();
	beginLine(4);
	moveTo(0, 389);
	moveTo(1023, 389);
	commit();

	beginLine(4);
	moveCircle(511, 389, zoom(3000), 16);
	commit();
	beginLine(4);
	moveCircle(511, 389, zoom(6000), 16);
	commit();
	beginLine(4);
	moveCircle(511, 389, zoom(12000), 16);
	commit();

	fflush(stdout);
}

static void
drawLidar(struct lidarPoint points[360])
{
	int j;
	double x1, y1, x2, y2;

	for (int i = 0; i < 360; i++)
		if (points[i].intensity < 64 || points[i].distance == 0)
			points[i].distance = 12000;
	for (int i = 0; i < 360; i++) {
		j = (i + 1) % 360;
		x1 = 511 + zoom(points[i].distance) * cos(deg2rad(i));
		y1 = 389 - zoom(points[i].distance) * sin(deg2rad(i));

		if (abs(points[i].distance - points[j].distance) >= 500) {
			x2 = x1;
			y2 = y1;
		} else {
			x2 = 511 + zoom(points[j].distance) * cos(deg2rad(j));
			y2 = 389 - zoom(points[j].distance) * sin(deg2rad(j));
		}

		beginLine(0);
		moveTo(x1, y1);
		moveTo(x2, y2);
		commit();
	}

	fflush(stdout);
}

static void
handler(int sig)
{
	longjmp(env, 1);
}

int
main(int argc, char *argv[])
{
	struct lidarPoint points[360];

	if (initLidar() == -1)
		err(1, "initLidar");

	if (setjmp(env) != 0)
		goto quit;
	(void)signal(SIGINT, handler);

	beginGraph();
	while (/* CONSTCOND */ 1) {
		usleep(100 * 1000);
		if (readLidar(points) == -1)
			err(1, "readLidar");
		clearGraph();
		drawBackground();
		drawLidar(points);
	}
quit:
	commit();
	endGraph();
	(void)endLidar();

	return 0;
}
