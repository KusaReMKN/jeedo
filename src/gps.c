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

#include <sys/socket.h>
#include <sys/types.h>

#include <errno.h>
#include <jansson.h>
#include <netdb.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dirdist.h"
#include "gps.h"

static pthread_mutex_t		mutex;
static volatile struct latlng	pos;
static int			running;
static pthread_t		watcher;

static void	cleanUpWatcher(void *arg);
static int	tcpConnect(const char *restrict host,
						const char *restrict serv);
static void *	watchGps(void *arg);

static void
cleanUpWatcher(void *arg)
{
	(void)close(*(int *)arg);
	running = 0;
}

int
endGps(void)
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
initGps(void)
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
	pos.lat = pos.lng = NAN;
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
	errno = pthread_create(&watcher, &attr, watchGps, NULL);
	if (errno != 0)
		goto fatal;
	/* Clean up */
	(void)pthread_attr_destroy(&attr);

	return 0;
}

int
readGps(struct latlng *p)
{
	if (!running) {
		errno = ESRCH;
		return -1;
	}
	errno = pthread_mutex_lock(&mutex);
	if (errno != 0)
		return -1;
	*p = pos;
	errno = pthread_mutex_unlock(&mutex);
	if (errno != 0)
		return -1;

	return 0;
}

static int
tcpConnect(const char *restrict host, const char *restrict serv)
{
	int sock;
	struct addrinfo hints, *result, *rp;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if (getaddrinfo(host, serv, &hints, &result) != 0)
		return -1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue;
		if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
			break;	/* success */
		(void)close(sock);
	}
	freeaddrinfo(result);
	if (rp == NULL)
		return -1;

	return sock;
}

static void *
watchGps(void *arg)
{
#define GPSD_HOST	"localhost"
#define GPSD_SERV	"2947"
#define GPSD_REQUEST	\
	"?WATCH={\"enable\":true,\"json\":true,\"scaled\",true}\n"
	int sock;
	json_t *lat, *lon, *root;
	time_t last;

	running = 1;
	sock = tcpConnect(GPSD_HOST, GPSD_SERV);
	if (sock == -1) {
		running = 0;
		return (void *)-1;
	}

	pthread_cleanup_push(cleanUpWatcher, &sock);
	if (write(sock, GPSD_REQUEST, strlen(GPSD_REQUEST)) == -1)
		goto fatal;

	while (/* CONSTCOND */ 1) {
		root = json_loadfd(sock, JSON_DISABLE_EOF_CHECK, NULL);
		if (root == NULL)
			break;
		lat = json_object_get(root, "lat");
		lon = json_object_get(root, "lon");
		if (lat != NULL && lon != NULL) {
			(void)pthread_mutex_lock(&mutex);
			pos.lat = json_number_value(lat);
			pos.lng = json_number_value(lon);
			(void)pthread_mutex_unlock(&mutex);
			time(&last);
		} else {
#define GPS_TIMEOUT	3	/* 3 sec */
			if (time(NULL) - last > GPS_TIMEOUT)
				break;
		}
		if (lat != NULL)
			json_decref(lat);
		if (lon != NULL)
			json_decref(lon);
		json_decref(root);
	}
fatal:	pthread_cleanup_pop(1);

	return (void *)-1;
}
