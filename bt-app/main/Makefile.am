AM_CPPFLAGS = -O2 \
              -D_GNU_SOURCE

AM_CFLAGS = -Wall \
        -Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -std=c99

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS += -I${WORKSPACE}/qcom-opensource/bt/bt-app \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/osi/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/gap/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/main/include \
               -I${WORKSPACE}/qcom-opensource/bt/bt-app/include \
               -I${WORKSPACE}/hardware/libhardware/include

c_sources = ../osi/src/alarm.c \
            ../osi/src/allocator.c \
            ../osi/src/allocation_tracker.c \
             ../osi/src/config.c \
            ../osi/src/fixed_queue.c \
            ../osi/src/hash_functions.c \
            ../osi/src/hash_map.c \
            ../osi/src/list.c \
            ../osi/src/reactor.c \
            ../osi/src/semaphore.c \
            ../osi/src/thread.c \
            ../osi/src/future.c \
            src/Main.cpp \
            src/ipc.c \
            ../gap/src/Gap.cpp \
            ../gap/src/AdapterProperties.cpp \
            ../gap/src/RemoteDevices.cpp

requiredlibs = -lpthread -lm -ldl -lrt -lhardware

AM_LDFLAGS := $(LDFLAFGS)

bin_PROGRAMS = btapp
btapp_CC = @CC@
btapp_SOURCES = $(c_sources)
btapp_LDADD = $(requiredlibs)
pkgconfigdir = $(libdir)/pkgconfig
