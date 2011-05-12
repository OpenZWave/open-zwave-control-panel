#
# Makefile for OpenzWave Control Panel application
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

INCLUDES := -I ../open-zwave/cpp/src -I ../open-zwave/cpp/src/command_classes/ \
	-I ../open-zwave/cpp/src/value_classes/ -I ../open-zwave/cpp/src/platform/ \
	-I ../open-zwave/cpp/src/platform/unix -I ../open-zwave/cpp/tinyxml/ \
	-I ../libmicrohttpd/src/include
# Remove comment below for gnutls support
GNUTLS := #-lgnutls
#LIBZWAVE := $(wildcard ../open-zwave/cpp/lib/linux/*.a)
#LIBUSB := -ludev
# Remove comment below for gnutls support
GNUTLS := #-lgnutls
# for Mac OS X comment out above 2 lines and uncomment next 2 lines
#LIBZWAVE := $(wildcard ../open-zwave/cpp/lib/mac/*.a)
#LIBUSB := -framework IOKit -framework CoreFoundation
LIBS := $(LIBZWAVE) $(GNUTLS) ../libmicrohttpd/src/daemon/.libs/libmicrohttpd.a -pthread $(LIBUSB)

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

ozwcp:	ozwcp.o webserver.o zwavelib.o
	$(LD) -o $@ $(LDFLAGS) ozwcp.o webserver.o zwavelib.o $(LIBS)
