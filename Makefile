CC = gcc
CFLAGS = -O2 -D_XOPEN_SOURCE=500 -D_FILE_OFFSET_BITS=64
LDFLAGS = -lcrypto
PROGRAM = tsanalytics
DEST = /usr/local/bin

# m2tlib関連
M2TLIB_SRCS := $(wildcard m2tlib/*.c)
M2TLIB_OBJS := m2tlib.o

# m2tlib以外の部分
SRCS := $(wildcard *.c)
OBJS := $(patsubst %.c,%.o,$(SRCS))

all:$(PROGRAM)

$(M2TLIB_OBJS):
	$(CC) $(CFLAGS) -r $(M2TLIB_SRCS) -o $(M2TLIB_OBJS) -nostdlib

$(PROGRAM): $(OBJS) $(M2TLIB_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(M2TLIB_OBJS)

.c:
	echo $(CC) $(CFLAGS) -c $<

clean:
	rm $(M2TLIB_OBJS) $(OBJS) $(PROGRAM)

