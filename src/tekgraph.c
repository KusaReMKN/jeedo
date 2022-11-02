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
#include <stdio.h>

#include "tekgraph.h"

int
beginGraph(void)
{
	return printf("\x1B[?38h");
}

int
clearGraph(void)
{
	return printf("\x1B\x0C\x0D\x0A");
}

int
endGraph(void)
{
	return printf("\x1B[?38l");
}

int
beginLine(int type)
{
	return printf("\x1D\x1B%c", type | 0x60);
}

int
moveTo(int x, int y)
{
	return printf("%c%c%c%c", y >> 5 & 0x1F | 0x20, y >> 0 & 0x1F | 0x60,
			x >> 5 & 0x1F | 0x20, x >> 0 & 0x1F | 0x40);
}

void
moveCircle(int x, int y, int r, int n)
{
	for (double k = 0; k <= 8.0; k += 2.0 / n)
		moveTo(x + r * cos(k * atan(1)), y + r * sin(k * atan(1)));
}

int
commit(void)
{
	return printf("\x1F");
}

int
beginCursor(void)
{
	return printf("\x1D");
}
