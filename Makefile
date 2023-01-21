# Makefile for libPlayerOneFW

CC = gcc
CFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
CPPFLAGS = -fPIC -Wall -Wextra -O2 -g -DSB_LINUX_BUILD -I. -I./../../
LDFLAGS = -L`pwd`/static_libs/`arch` -shared -lstdc++ -lPlayerOnePW
RM = rm -f
STRIP = strip
TARGET_LIB = libPlayerOneFW.so

SRCS = main.cpp x2filterwheel.cpp PlayerOneFW.cpp
OBJS = $(SRCS:.cpp=.o)

.PHONY: all
all: ${TARGET_LIB}

$(TARGET_LIB): $(OBJS)
	$(CC) ${LDFLAGS} -o $@ $^
	patchelf --add-needed  libPlayerOnePW.so.1.0.0 $@
	$(STRIP) $@ >/dev/null 2>&1  || true

$(SRCS:.cpp=.d):%.d:%.cpp
	$(CC) $(CFLAGS) $(CPPFLAGS) -MM $< >$@

.PHONY: clean
clean:
	${RM} ${TARGET_LIB} ${OBJS} *.d
