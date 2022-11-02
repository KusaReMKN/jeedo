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

#include <math.h>

#include "debug.h"	/* 85, 109 */
#include "dirdist.h"

#define PI	/* PI */	\
	3.141592653589793238462643383279502884197169399375105820974944e+00
#define PI_2	/* PI/2 */	\
	1.570796326794896619231321691639751442098584699687552910487472e+00
#define PI_180	/* PI/180 */	\
	1.745329251994329576923690768488612713442871888541725456097191e-02

#define deg2rad(deg)	((deg) * PI_180)
#define rad2deg(rad)	((rad) / PI_180)

#define RNeE	6378137.000	/* Nominal equatorial Earth radius */
#define RNpE	6356752.314	/* Nominal polar Earth radius */
#define EE	/* Eccentricity^2; ((RNeE^2 - RNpE^2) / RNeE^2) */	\
	6.694380066764775264607878911781579375347155603499272940882800e-03
#define RoPMC(alat)	/* Radius of prime meridian circle */	\
	((RNeE * (1 - EE)) / pow(1 - EE * sin(alat) * sin(alat), 1.5))
#define RoPVC(alat)	/* Radius of prime vertical circle */	\
	(RNeE / sqrt(1 - EE * sin(alat) * sin(alat)))

static void	moveBy(struct latlng *p, double distance, double direction);
static double	normalizeLat(double lat);
static double	normalizeLng(double lng);

double
direction(const struct latlng *restrict p, const struct latlng *restrict q)
{
	double a, b, d, r;

	a = deg2rad(p->lat);
	b = deg2rad(q->lat);
	d = deg2rad(q->lng - p->lng);
	r = PI_2 - atan2(cos(a)*tan(b) - sin(a)*cos(d), sin(d));

	return normalize(rad2deg(r));
}

double
distance(const struct latlng *restrict p, const struct latlng *restrict q)
{
	double alat, dlat, dlng;

	alat = deg2rad((q->lat + p->lat) / 2);
	dlat = deg2rad(q->lat - p->lat);
	dlng = deg2rad(q->lng - p->lng);

	return sqrt(pow(dlat * RoPMC(alat), 2)
			+ pow(dlng * RoPVC(alat) * cos(alat), 2));
}

int
isDirected(double a, double b)
{
#define DIRECTED_THRESHOLD	15.0	/* FIXME */
	DEBUG_printf("direction: %f\n", a - b);
	return fabs(a - b) < DIRECTED_THRESHOLD;
}

int
isOutside(const struct latlng *restrict p, double heading,
		const struct latlng *restrict q)
{
#define TURNING_RADIUS	2.0
	double dir;
	struct latlng center;

	center = *p;
	dir = normalize(heading - direction(p, q)) < 180 ? -90 : 90;
	moveBy(&center, TURNING_RADIUS, heading + dir);

	return distance(&center, q) >= TURNING_RADIUS;
}

int
isReached(const struct latlng *restrict p, const struct latlng *restrict q)
{
#define REACHED_THRESHOLD	0.3	/* FIXME */
	double temp = distance(p, q);
	DEBUG_printf("distance: %f\n", temp);
	return temp < REACHED_THRESHOLD;
}

static void
moveBy(struct latlng *p, double distance, double direction)
{
	double clat, dlat, dlng;

	clat = deg2rad(p->lat);
	dlat = distance * cos(deg2rad(direction)) / RoPMC(clat);
	dlng = distance * sin(deg2rad(direction)) / (RoPVC(clat) * cos(clat));
	p->lat = normalizeLat(p->lat + rad2deg(dlat));
	p->lng = normalizeLng(p->lng + rad2deg(dlng));
}

double
normalize(double deg)
{
	while (deg > 360)
		deg -= 360;
	while (deg < 0)
		deg += 360;
	return deg;
}

static double
normalizeLat(double lat)
{
	while (lat > 90)
		lat -= lat - 90;
	while (lat < -90)
		lat -= lat + 90;
	return lat;
}

static double
normalizeLng(double lng)
{
	while (lng > 180)
		lng -= 360;
	while (lng < -180)
		lng += 360;
	return lng;
}
