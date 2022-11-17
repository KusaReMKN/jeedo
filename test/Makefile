#! /usr/bin/make -f
#
# Copyright (c) 2022, KusaReMKN.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

TARGETS=	compass_test lidar_test nowave

CFLAGS=		-I../src -O2 -Wall
RM=		rm -f
SHELL=		/bin/sh
VPATH=		../src

all:	$(TARGETS)

compass_test:	compass_test.o compass.o i2c.o
	$(CC) -o $@ $^

lidar_test:	lidar_test.o lidar.o tekgraph.o
	$(CC) -o $@ $^ -lm -pthread

nowave:		nowave.o compass.o denkino.o dirdist.o drive.o gps.o i2c.o \
		lidar.o
	$(CC) -o $@ $^ -lm -ljansson -pthread

.PHONY: clean
clean:
	$(RM) *.o $(TARGETS)
