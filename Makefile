#
# Makefile for OpenzWave Control Panel application
# Greg Satz

# GNU make only

.SUFFIXES:	.cpp .o .a .s

CC     := $(CROSS_COMPILE)gcc
CXX    := $(CROSS_COMPILE)g++
LD     := $(CROSS_COMPILE)g++
AR     := $(CROSS_COMPILE)ar rc
RANLIB := $(CROSS_COMPILE)ranlib

DEBUG_CFLAGS    := -Wall -Wno-unknown-pragmas -Wno-inline -Wno-format -g -DDEBUG -ggdb -O0 -std=c++11
RELEASE_CFLAGS  := -Wall -Wno-unknown-pragmas -Werror -Wno-format -O3 -DNDEBUG -std=c++11

DEBUG_LDFLAGS	:= -g

# Change for DEBUG or RELEASE
CFLAGS	:= -c $(DEBUG_CFLAGS)
LDFLAGS	:= $(DEBUG_LDFLAGS) -v

LIBMICROHTTPD := -L/usr/local/lib/ -lmicrohttpd

# Remove comment below for gnutls support
#GNUTLS := -lgnutls

# Determine the system we are building on
UNAME  := $(shell uname -s)

# Select proper options for macOS vs others
ifeq ($(UNAME),Darwin)
$(info setting variables for macOS)
ARCH := -arch x86_64
CFLAGS += $(ARCH)
LIBZWAVE_PAT := libopenzwave.dylib
LIBUSB := -framework IOKit -framework CoreFoundation
else
$(info setting variables for $(UNAME))
LIBZWAVE_PAT := *.a
LIBUSB := -ludev
endif

# Check if library exists in either parrent directory or ../open-zwave
OPENZWAVE := ..
LIBZWAVE := $(wildcard $(OPENZWAVE)/$(LIBZWAVE_PAT))
ifeq ($(LIBZWAVE),)
  OPENZWAVE := ../open-zwave
  LIBZWAVE := $(wildcard $(OPENZWAVE)/$(LIBZWAVE_PAT))
  ifeq ($(LIBZWAVE),)
    $(error OpenZWave library "$(LIBZWAVE_PAT)" not found in either .. or ../open-zwave folder)
  endif
endif
$(info OpenZWave library found: $(LIBZWAVE))

LIBS := $(LIBZWAVE) $(GNUTLS) $(LIBMICROHTTPD) -pthread $(LIBUSB) $(ARCH) -lresolv

INCLUDES := -I $(OPENZWAVE)/cpp/src -I $(OPENZWAVE)/cpp/src/command_classes/ \
	-I $(OPENZWAVE)/cpp/src/value_classes/ -I $(OPENZWAVE)/cpp/src/platform/ \
	-I $(OPENZWAVE)/cpp/src/platform/unix -I $(OPENZWAVE)/cpp/tinyxml/ \
	-I /usr/local/include/

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $<

all: ozwcp

ozwcp:	ozwcp.o webserver.o zwavelib.o $(LIBZWAVE)
	$(LD) -o $@ $(LDFLAGS) ozwcp.o webserver.o zwavelib.o $(LIBS)

dist:	ozwcp
	rm -f ozwcp.tar.gz
	tar -c --exclude=".svn" -hvzf ozwcp.tar.gz ozwcp config/ cp.html cp.js openzwavetinyicon.png README

clean:
	rm -f ozwcp *.o

# dummy target, for debugging Makefile
dummy: