# tty_week version
VERSION = 1.0

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# flags
# We use curl for HTTP requests in the daemon
LIBS = -lcurl
CPPFLAGS = -DVERSION=\"${VERSION}\" -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c99 -pedantic -Wall -O2 ${CPPFLAGS}
LDFLAGS = ${LIBS}

# compiler and linker
CC = cc
