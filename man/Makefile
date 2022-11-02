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

TARGETS=	compass.pdf \
		debug.pdf \
		denkino.pdf \
		dirdist.pdf dirdist_s.pdf \
		drive.pdf \
		gps.pdf \
		i2c.pdf \
		lidar.pdf

GROFF=		LC_CTYPE=ja_JP.eucJP groff
GROFFFLAGS=	-P-pa4 -ept -dlang=ja_JP.eucJP -ma4 -mandoc
PS2PDF=		ps2pdf
PS2PDFFLAGS=	-sPAPERSIZE=a4
RM=		rm -f
SHELL=		/bin/sh

all: $(TARGETS)

.SUFFIXES:
.SUFFIXES: .3 .pdf

.3.pdf:
	$(GROFF) $(GROFFFLAGS) -Tps $< | $(PS2PDF) $(PS2PDFFLAGS) - $@

.PHONY: clean
clean:
	$(RM) $(TARGETS)
