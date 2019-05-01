# This file is part of Vamps.
#
# Vamps is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; version 2 of the License.
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
CFLAGS    ?= -pipe -O2 -fomit-frame-pointer
CFLAGS    += -Wall -W
LDFLAGS   += -s
#LDFLAGS  += -static
LOADLIBES += -ldvdread

INSTALL   ?= install


all:		play_cell

play_cell:	play_cell.o

install:	all
		$(INSTALL) -d $(PREFIX)/bin
		$(INSTALL) -m 755 play_cell $(PREFIX)/bin

uninstall:
		rm -f $(PREFIX)/bin/play_cell

clean:
		rm -f play_cell play_cell.o *~ core
