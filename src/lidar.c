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

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "errwarn.h"
#include "lidar.h"

#define LIDAR_PATH	"/dev/ttyUSB0"

struct lidarPacket {
	uint8_t  header;	/* always 0x54 */
	uint8_t  verlen;	/* always 0x2C */
	uint16_t speed;		/* speed [deg/sec] */
	uint16_t start;		/* start angle [0.01 deg] */
#define POINTS_PER_PACKET	12
	struct lidarPoint points[POINTS_PER_PACKET];	/* point data */
	uint16_t end;		/* end angle [0.01 deg] */
	uint16_t timestamp;	/* time stamp [%30000 ms] */
	uint8_t crc;		/* CRC checksum */
} __attribute__((__packed__));

struct lidarState {
	int fd;
	struct termios *prev;
};

static const uint8_t crcTab[256] = {
	0x00, 0x4D, 0x9A, 0xD7,    0x79, 0x34, 0xE3, 0xAE,
	0xF2, 0xBF, 0x68, 0x25,    0x8B, 0xC6, 0x11, 0x5C,
	0xA9, 0xE4, 0x33, 0x7E,    0xD0, 0x9D, 0x4A, 0x07,
	0x5B, 0x16, 0xC1, 0x8C,    0x22, 0x6F, 0xE8, 0xF5,

	0x1F, 0x52, 0x85, 0xC8,    0x66, 0x2B, 0xFC, 0xB1,
	0xED, 0xA0, 0x77, 0x3A,    0x94, 0xD9, 0x0E, 0x43,
	0xB6, 0xFB, 0x2C, 0x61,    0xCF, 0x82, 0x55, 0x18,
	0x44, 0x09, 0xDE, 0x93,    0x3D, 0x70, 0xA7, 0xEA,

	0x3E, 0x73, 0xA4, 0xE9,    0x47, 0x0A, 0xDD, 0x90,
	0xCC, 0x81, 0x56, 0x1B,    0xB5, 0xF8, 0x2F, 0x62,
	0x97, 0xDA, 0x0D, 0x40,    0xEE, 0xA3, 0x74, 0x39,
	0x65, 0x28, 0xFF, 0xB2,    0x1C, 0x51, 0x86, 0xCB,

	0x21, 0x6C, 0xBB, 0xF6,    0x58, 0x15, 0xC2, 0x8F,
	0xD3, 0x9E, 0x49, 0x04,    0xAA, 0xE7, 0x30, 0x7D,
	0x88, 0xC5, 0x12, 0x5F,    0xF1, 0xBC, 0x6B, 0x26,
	0x7A, 0x37, 0xE0, 0xAD,    0x03, 0x4E, 0x99, 0xD4,

	0x7C, 0x31, 0xE6, 0xAB,    0x05, 0x48, 0x9F, 0xD2,
	0x8E, 0xC3, 0x14, 0x59,    0xF7, 0xBA, 0x6D, 0x20,
	0xD5, 0x98, 0x4F, 0x02,    0xAC, 0xE1, 0x36, 0x7B,
	0x27, 0x6A, 0xBD, 0xF0,    0x5E, 0x13, 0xC4, 0x89,

	0x63, 0x2E, 0xF9, 0xB4,    0x1A, 0x57, 0x80, 0xCD,
	0x91, 0xDC, 0x0B, 0x46,    0xE8, 0xA5, 0x72, 0x3F,
	0xCA, 0x87, 0x50, 0x1D,    0xB3, 0xFE, 0x29, 0x64,
	0x38, 0x75, 0xA2, 0xEF,    0x41, 0x0C, 0xDB, 0x96,

	0x42, 0x0F, 0xD8, 0x95,    0x3B, 0x76, 0xA1, 0xEC,
	0xB0, 0xFD, 0x2A, 0x67,    0xC9, 0x84, 0x53, 0x1E,
	0xEB, 0xA6, 0x71, 0x3C,    0x92, 0xDF, 0x08, 0x45,
	0x19, 0x54, 0x83, 0xCE,    0x60, 0x2D, 0xFA, 0xB7,

	0x5D, 0x10, 0xC7, 0x8A,    0x24, 0x69, 0xBE, 0xF3,
	0xAF, 0xE2, 0x35, 0x78,    0xD6, 0x9B, 0x4C, 0x01,
	0xF4, 0xB9, 0x6E, 0x23,    0x8D, 0xC0, 0x17, 0x5A,
	0x06, 0x4B, 0x9C, 0xD1,    0x7F, 0x32, 0xE5, 0xA8,
};

static volatile struct lidarPoint points[360];
static pthread_mutex_t	mutex;
static int		running;
static pthread_t	watcher;

static int	checkPacket(const struct lidarPacket *packet);
static void	cleanUpWatcher(void *arg);
static int	closeLidar(int fd, const struct termios *prev);
static int	openLidar(struct termios *prev);
static int	readPacket(int fd, struct lidarPacket *packet);
static void *	watchLidar(void *arg);

static int
checkPacket(const struct lidarPacket *packet)
{
	const unsigned char *p;
	int crc, i;

	p = (const unsigned char *)packet;
	crc = 0;
	for (i = 0; i < sizeof(*packet) - 1; i++)
		crc = crcTab[crc ^ *p++];

	return (crc & 0xFF) == packet->crc;
}

static void
cleanUpWatcher(void *arg)
{
	struct lidarState *ls;

	ls = arg;
	(void)closeLidar(ls->fd, ls->prev);
	running = 0;
}

static int
closeLidar(int fd, const struct termios *prev)
{
	if (prev != NULL)
		if (tcsetattr(fd, TCSANOW, prev) == -1)
			return -1;
	return close(fd);
}

int
endLidar(void)
{
	errno = pthread_cancel(watcher);
	if (errno != 0)
		return -1;
	errno = pthread_join(watcher, NULL);
	if (errno != 0)
		return -1;
	errno = pthread_mutex_destroy(&mutex);
	if (errno != 0)
		return -1;

	return 0;
}

int
initLidar(void)
{
	int errsv, max, min;
	pthread_attr_t attr;
	struct sched_param param;

	/* Already running? */
	if (running) {
		errno = EBUSY;
		return -1;
	}
	/* Initiate the mutex */
	errno = pthread_mutex_init(&mutex, NULL);
	if (errno != 0)
		return -1;
	/* Set the priority of the watcher thread */
	errno = pthread_attr_init(&attr);
	if (errno != 0) {
		errsv = errno;
		(void)pthread_mutex_destroy(&mutex);
		errno = errsv;
		return -1;
	}
	errno = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (errno != 0) {
fatal:		errsv = errno;
		(void)pthread_mutex_destroy(&mutex);
		(void)pthread_attr_destroy(&attr);
		errno = errsv;
		return -1;
	}
	errno = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (errno != 0)
		goto fatal;
	max = sched_get_priority_max(SCHED_FIFO);
	if (max == -1)
		goto fatal;
	min = sched_get_priority_min(SCHED_FIFO);
	if (min == -1)
		goto fatal;
	param.sched_priority = (max + min) / 2;
	errno = pthread_attr_setschedparam(&attr, &param);
	if (errno != 0)
		goto fatal;
	/* Create the watcher thread */
	errno = pthread_create(&watcher, &attr, watchLidar, NULL);
	if (errno != 0)
		goto fatal;
	/* Clean up */
	(void)pthread_attr_destroy(&attr);

	return 0;
}

static int
openLidar(struct termios *prev)
{
	int errsv, fd;
	struct termios tio;

	/* Try to open the USB serial port */
	fd = open(LIDAR_PATH, O_RDONLY | O_NOCTTY);
	if (fd == -1)
		return -1;
	/* Get the previous attributes */
	if (tcgetattr(fd, &tio) == -1) {
fatal:		errsv = errno;
		(void)close(fd);
		errno = errsv;
		return -1;
	}
	if (prev != NULL)
		memcpy(prev, &tio, sizeof(*prev));
	/* Set the new attributes */
	cfmakeraw(&tio);
	(void)cfsetspeed(&tio, B230400);
	tio.c_cflag = (tio.c_cflag & ~CSIZE) | CS8;
	tio.c_cc[VMIN] = sizeof(struct lidarPacket);
#define LIDAR_TIMEOUT	10	/* 1 sec */
	tio.c_cc[VTIME] = LIDAR_TIMEOUT;
	if (tcsetattr(fd, TCSANOW, &tio) == -1)
		goto fatal;

	return fd;
}

int
readLidar(struct lidarPoint buf[360])
{
	if (!running) {
		errno = ESRCH;
		return -1;
	}
	errno = pthread_mutex_lock(&mutex);
	if (errno != 0)
		return -1;
	memcpy(buf, (struct lidarPoint *)points, sizeof(points));
	errno = pthread_mutex_unlock(&mutex);
	if (errno != 0)
		return -1;

	return 0;
}

static int
readPacket(int fd, struct lidarPacket *packet)
{
	ssize_t cur, nRead;

	do {
		do {
			nRead = read(fd, packet, 1);
			if (nRead == -1)
				return -1;
		} while (packet->header != 0x54);
		cur = nRead;
		while (cur < sizeof(*packet)) {
			nRead = read(fd, (char *)packet + cur,
					sizeof(*packet) - cur);
			if (nRead == 0)
				errno = EREMOTEIO;
			if (nRead == -1 || nRead == 0)
				return -1;
			cur += nRead;
		}
	} while (!checkPacket(packet));

	return 0;
}

static void *
watchLidar(void *arg)
{
	int fd, i, index;
	struct termios prev;
	struct lidarState ls;
	struct lidarPacket packet;
	double baseAngle, dAngle;

	running = 1;
	fd = openLidar(&prev);
	if (fd == -1) {
		warning("openLidar: %s\n", strerror(errno));
		running = 0;
		return (void *)-1;
	}
	ls.fd = fd, ls.prev = &prev;

	pthread_cleanup_push(cleanUpWatcher, &ls);
	while (/* CONSTCOND */ 1) {
		if (readPacket(fd, &packet) == -1) {
			warning("readPacket: %s\n", strerror(errno));
			break;
		}
		if (packet.start > packet.end)
			packet.end += 36000;
		dAngle = (packet.end - packet.start) * 0.01
				/ (POINTS_PER_PACKET - 1);
		baseAngle = packet.start * 0.01;
		(void)pthread_mutex_lock(&mutex);
		for (i = 0; i < POINTS_PER_PACKET; i++) {
			index = baseAngle + dAngle * i + 0.5;	/* rounded */
			index = (360 + index) % 360;
			points[index] = packet.points[i];
		}
		(void)pthread_mutex_unlock(&mutex);
	}
	pthread_cleanup_pop(1);

	return (void *)-1;
}
