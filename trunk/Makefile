#
# Makefile for OpenzWave Mac OS X applications
# Greg Satz

# GNU make only

.SUFFIXES:	.cpp .o .a .s

CC     := gcc
CXX    := g++
LD     := g++
AR     := ar rc
RANLIB := ranlib

DEBUG_CFLAGS    := -Wall -Wno-format -g -DDEBUG
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

INCLUDES	:= -I ../open-zwave/cpp/src -I ../open-zwave/cpp/src/command_classes/ \
	-I ../open-zwave/cpp/src/value_classes/ -I ../open-zwave/cpp/src/platform/ \
	-I ../open-zwave/cpp/src/platform/unix -I ../open-zwave/cpp/tinyxml/ \
	-I ../libmicrohttpd/src/include
LIBS := $(wildcard ../open-zwave/cpp/lib/linux/*.a) ../libmicrohttpd/src/daemon/.libs/libmicrohttpd.a

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

all: ozwcp

ozwcp.o: ozwcp.h webserver.h ../open-zwave/cpp/src/Options.h ../open-zwave/cpp/src/Manager.h \
	../open-zwave/cpp/src/Node.h ../open-zwave/cpp/src/Group.h \
	../open-zwave/cpp/src/Notification.h ../open-zwave/cpp/src/platform/Log.h

webserver.o: webserver.h ozwcp.h ../open-zwave/cpp/src/Options.h ../open-zwave/cpp/src/Manager.h \
	../open-zwave/cpp/src/Node.h ../open-zwave/cpp/src/Group.h \
	../open-zwave/cpp/src/Notification.h ../open-zwave/cpp/src/platform/Log.h

ozwcp:	ozwcp.o webserver.o zwavelib.o $(LIBS)
	$(LD) -o $@ $(LDFLAGS) ozwcp.o webserver.o zwavelib.o $(LIBS) -pthread -ludev
