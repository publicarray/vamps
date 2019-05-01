# This file is part of Vamps.
#
# Vamps is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Vamps is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Vamps; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


PREFIX    ?= /usr/local

CC         = gcc
CFLAGS    ?= -pipe -O3 -fomit-frame-pointer -funroll-loops \
	       -DHAVE_BUILTIN_EXPECT
CFLAGS    += -Wall
LDFLAGS   += -s
#LDFLAGS  += -static
LOADLIBES += -lm -lpthread

INSTALL   ?= install


all:		vamps

vamps:		vamps.o requant.o

requant.o:	requant.h putvlc.h getvlc.h qTable.h

install:	all
		$(INSTALL) -d $(PREFIX)/bin
		$(INSTALL) -m 755 vamps $(PREFIX)/bin

uninstall:
		rm -f $(PREFIX)/bin/vamps

clean:
		rm -f vamps vamps.o requant.o *~ core
