# -----------------------------------------------------------------------------
# Copyright telecnatron.com. 2014.
# $Id: $
# Be sure to call /home/steves/bin/openwrt-compile script to config environment prior to calling make
# -----------------------------------------------------------------------------
TOOLCHAIN_DIR=/usr/local/cross-compile/openwrt/staging_dir/toolchain-mips_34kc_gcc-4.8-linaro_uClibc-0.9.33.2
INCLUDE_DIR=$(TOOLCHAIN_DIR)/usr/include

# -----------------------------------------------------------------------------
# Source and header files are listed here
# BIN_NAME is the name of the binary executable that will be produced, and the name of the source file (with .c appended)
BIN_NAME=sbread
# list source files (.c) here
SOURCES=$(BIN_NAME).c logger.c crc.c
INCLUDES=
# -----------------------------------------------------------------------------

CC=mips-openwrt-linux-gcc
CFLAGS= -std=gnu99 
LDFLAGS=-lbluetooth -lm
DEFS=
OBJS=$(SOURCES:.c=.o) 
# -----------------------------------------------------------------------------
all: $(BIN_NAME)

$(BIN_NAME).o: $(BIN_NAME).c $(INCLUDES)
	$(CC) -c $(CFLAGS) $(DEFS) -I $(INCLUDE_DIR) -o $@ $<

%.o: %.c %.h 
	$(CC) -c $(CFLAGS) $(DEFS) -I $(INCLUDE_DIR) -o $@ $<

$(BIN_NAME): $(OBJS)
	$(CC) $(LDFLAGS) $(CFLAGS) -o $(BIN_NAME) $(OBJS)

clean:
	rm *.o $(BIN_NAME)