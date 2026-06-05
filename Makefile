# tty_week - suckless TTY week timer
# See LICENSE file for copyright and license details.

include config.mk

SRC = tty_week.c tty_weekd.c
OBJ = ${SRC:.c=.o}

all: tty_week tty_weekd

.c.o:
	${CC} -c ${CFLAGS} $<

${OBJ}: config.h config.mk

config.h:
	cp config.def.h $@

tty_week: tty_week.o
	${CC} -o $@ tty_week.o

tty_weekd: tty_weekd.o
	${CC} -o $@ tty_weekd.o ${LDFLAGS}

clean:
	rm -f tty_week tty_weekd ${OBJ} tty_week-${VERSION}.tar.gz

dist: clean
	mkdir -p tty_week-${VERSION}
	cp -R LICENSE Makefile config.mk config.def.h \
		${SRC} tty_week-${VERSION}
	tar -cf tty_week-${VERSION}.tar tty_week-${VERSION}
	gzip tty_week-${VERSION}.tar
	rm -rf tty_week-${VERSION}

install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f tty_week tty_weekd ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/tty_week
	chmod 755 ${DESTDIR}${PREFIX}/bin/tty_weekd

uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/tty_week
	rm -f ${DESTDIR}${PREFIX}/bin/tty_weekd

.PHONY: all clean dist install uninstall
