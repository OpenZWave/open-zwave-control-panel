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

DEBUG_CFLAGS    := -Wall -Wno-inline -Wno-format -g -DDEBUG -Werror
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Wno-format -O3 -DNDEBUG

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS)

OPENZWAVE := ../open-zwave
LIBMICROHTTPD := ../libmicrohttpd/src/daemon/.libs/libmicrohttpd.a

INCLUDES := -I $(OPENZWAVE)/cpp/src -I $(OPENZWAVE)/cpp/src/command_classes/ \
	-I $(OPENZWAVE)/cpp/src/value_classes/ -I $(OPENZWAVE)/cpp/src/platform/ \
	-I $(OPENZWAVE)/cpp/src/platform/unix -I $(OPENZWAVE)/cpp/tinyxml/ \
	-I ../libmicrohttpd/src/include

# Remove comment below for gnutls support
GNUTLS := #-lgnutls

# for Linux uncomment out next two lines
#LIBZWAVE := $(wildcard $(OPENZWAVE)/cpp/lib/linux/*.a)
#LIBUSB := -ludev

# for Mac OS X comment out above 2 lines and uncomment next 2 lines
#LIBZWAVE := $(wildcard $(OPENZWAVE)/cpp/lib/mac/*.a)
#LIBUSB := -framework IOKit -framework CoreFoundation

LIBS := $(LIBZWAVE) $(GNUTLS) $(LIBMICROHTTPD) -pthread $(LIBUSB)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

all: ozwcp

ozwcp.o: ozwcp.h webserver.h $(OPENZWAVE)/cpp/src/Options.h $(OPENZWAVE)/cpp/src/Manager.h \
	$(OPENZWAVE)/cpp/src/Node.h $(OPENZWAVE)/cpp/src/Group.h \
	$(OPENZWAVE)/cpp/src/Notification.h $(OPENZWAVE)/cpp/src/platform/Log.h

webserver.o: webserver.h ozwcp.h $(OPENZWAVE)/cpp/src/Options.h $(OPENZWAVE)/cpp/src/Manager.h \
	$(OPENZWAVE)/cpp/src/Node.h $(OPENZWAVE)/cpp/src/Group.h \
	$(OPENZWAVE)/cpp/src/Notification.h $(OPENZWAVE)/cpp/src/platform/Log.h

ozwcp:	ozwcp.o webserver.o zwavelib.o
	$(LD) -o $@ $(LDFLAGS) ozwcp.o webserver.o zwavelib.o $(LIBS)

dist:	ozwcp
	rm -f ozwcp.tar.gz
	tar -c --exclude=".svn" -hvzf ozwcp.tar.gz ozwcp config/ cp.html cp.js openzwavetinyicon.png README

clean:
	rm -f ozwcp *.o
