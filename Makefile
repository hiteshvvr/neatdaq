#####################################################################
#
#  Name:         Makefile
#  Created by:   Stefan Ritt
#
#  Contents:     Makefile for MIDAS example frontend and analyzer
#
#  $Id: Makefile 4647 2009-12-08 07:52:56Z ritt $
#
#####################################################################
#
#--------------------------------------------------------------------
# The MIDASSYS should be defined prior the use of this Makefile
ifndef MIDASSYS
missmidas::
	@echo "...";
	@echo "Missing definition of environment variable 'MIDASSYS' !";
	@echo "...";
endif

# get OS type from shell
OSTYPE = $(shell uname)

#--------------------------------------------------------------------
# The following lines contain specific switches for different UNIX
# systems. Find the one which matches your OS and outcomment the 
# lines below.

#-----------------------------------------
# This is for Linux
ifeq ($(OSTYPE),Linux)
OSTYPE = linux
endif


ifeq ($(OSTYPE),linux)

OS_DIR = linux
OSFLAGS = -DOS_LINUX -Dextname -DLINUX
CFLAGS = -g -O2 -Wall -fpermissive
# add to compile in 32-bit mode
# OSFLAGS += -m32
LIBS = -lm -lz -lutil -lnsl -lpthread -lrt -l CAENVME
endif

#-----------------------
# MacOSX/Darwin is just a funny Linux
#
ifeq ($(OSTYPE),Darwin)
OSTYPE = darwin
endif

ifeq ($(OSTYPE),darwin)
OS_DIR = darwin
FF = cc
OSFLAGS = -DOS_LINUX -DOS_DARWIN -DHAVE_STRLCPY -DAbsoftUNIXFortran -fPIC -Wno-unused-function
LIBS = -lpthread
SPECIFIC_OS_PRG = $(BIN_DIR)/mlxspeaker
NEED_STRLCPY=
NEED_RANLIB=1
NEED_SHLIB=
NEED_RPATH=

endif

#-----------------------------------------
# ROOT flags and libs
#
ifdef ROOTSYS
ROOTCFLAGS := $(shell  $(ROOTSYS)/bin/root-config --cflags)
ROOTCFLAGS += -DHAVE_ROOT -DUSE_ROOT
ROOTLIBS   := $(shell  $(ROOTSYS)/bin/root-config --libs) -Wl,-rpath,$(ROOTSYS)/lib
ROOTLIBS   += -lThread
else
missroot:
	@echo "...";
	@echo "Missing definition of environment variable 'ROOTSYS' !";
	@echo "...";
endif
#-------------------------------------------------------------------
# The following lines define directories. Adjust if necessary
#                 
DRV_DIR   = $(MIDASSYS)/drivers/camac
VME_DIR   = $(MIDASSYS)/drivers/vme
VMIC_DIR  = $(MIDASSYS)/drivers/vme/vmic
INC_DIR   = $(MIDASSYS)/include
LIB_DIR   = $(MIDASSYS)/$(OS_DIR)/lib
SRC_DIR   = $(MIDASSYS)/src

#-------------------------------------------------------------------
# List of analyzer modules
#
#MODULES   = adccalib.o adcsum.o tdccalib.o tdcsum.o scaler.o

#-------------------------------------------------------------------
# Hardware driver can be (camacnul, kcs2926, kcs2927, hyt1331)
#
DRIVER = camacnul
CAENINC = /usr/include/CAENVMElib.h /usr/include/CAENVMEtypes.h /usr/include/CAENVMEoslib.h

#OBJ = $(CAENINC) v2718.o v965.o v1190B.o v792.o v262.o v1290N.o MQDC.o v812.o #pvmeMidassub.o vmicvme.o v767.o
OBJ = $(CAENINC) v2718.o  v1190B.o v2495.o v1290N.o v1720.o v812.o #pvmeMidassub.o vmicvme.o v767.o

#-------------------------------------------------------------------
# Frontend code name defaulted to frontend in this example.
# comment out the line and run your own frontend as follow:
# gmake UFE=my_frontend
#
UFE = frontend

####################################################################
# Lines below here should not be edited
####################################################################

# MIDAS library
LIB = $(LIB_DIR)/libmidas.a

# PVIC specific OS.
#
OS          = Linux
CFLAGS_OS   = -D$(OS)

DIR_PVIC       = /opt/CES/pvic
DIR_LIBPVIC    = $(DIR_PVIC)/common/libpvic
DIR_INCLUDE    = $(DIR_PVIC)/common/include
DIR_INCLUDE_OS = $(DIR_PVIC)/Linux/include
DIR_EXAMPLES   = $(DIR_PVIC)/common/examples
DIR_DRIVER     = $(DIR_PVIC)/Linux/driver

# compiler
CC = gcc
CXX = g++
CFLAGS += -g -I$(INC_DIR) -I$(VME_DIR) -I$(VMIC_DIR)
LDFLAGS += 
INCLUDES = -I$(DIR_INCLUDE_OS) -I. -I$(DIR_INCLUDE) -I$(DIR_DRIVER)

# Shared or static library?
#
ifdef use-shared
  LIBPVIC   = libpvic.so
  CFLAGS_SH = -fPIC
else
  LIBPVIC   = libpvic.a
  CFLAGS_SH =
endif

# Libraries needed to build example programs.
#
LIBS2 = $(LIBPVIC)

# Collect compiler flags (CFLAGS).
#
ifdef use-debug
  CFLAGS2 = -O 
else
  CFLAGS2 = -O2
endif

# The -fvolatile flag ensures that D32 accesses are not 
# optimized to D8 or D16 accesses for expressions "a |= b".  
CFLAGS2  += -fvolatile $(CFLAGS_OS)
WARNINGS = -Wall -Wshadow -Wstrict-prototypes -Wmissing-prototypes

# Include PVIC source and header file list.
#
#include $(DIR_LIBPVIC)/Make.include
#include $(DIR_EXAMPLES)/Make.include

all: $(UFE) 
#analyzer

$(UFE): $(LIB) $(LIB_DIR)/mfe.o $(OBJ) $(DRIVER).o $(UFE).c $(SRC_DIR)/cnaf_callback.c $(UFE).h
	$(CC) $(CFLAGS) $(OSFLAGS) -o $(UFE) $(UFE).c \
	$(SRC_DIR)/cnaf_callback.c $(OBJ) $(DRIVER).o $(LIB_DIR)/mfe.o $(LIB) \
	$(LIBS) #$(LIBS2)

$(DRIVER).o: $(DRV_DIR)/$(DRIVER).c
	$(CC) $(CFLAGS) $(OSFLAGS) -c -o $(DRIVER).o $(DRV_DIR)/$(DRIVER).c

pvmeMidassub.o: pvmeMidassub.c pvmeMidas.h
	$(CC) -c $(CFLAGS2) -g $(INCLUDES) $(WARNINGS) $<

vmicvme.o: $(VME_DIR)/vmic/vmicvme.c 
	$(CC) -c $(CFLAGS) $(CFLAGS2)  $<

v1190B.o: $(VME_DIR)/v1190B.c
	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# v792.o: $(VME_DIR)/v792.c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# v965.o: $(VME_DIR)/v965.c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# MQDC.o: $(VME_DIR)/MQDC.c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# v767.o: $(VME_DIR)/v767.c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# v262.o: $(VME_DIR)/v262.c
# 	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

v2495.o: $(VME_DIR)/v2495.c
	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

v1290N.o: $(VME_DIR)/v1290N.c
	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

v1720.o: $(VME_DIR)/v1720.c
	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

v812.o: $(VME_DIR)/v812.c
	$(CC) $(CFLAGS) $(OSFLAGS) -Wno-unused-but-set-variable -Wno-unused-variable -o $@ -c $<

# analyzer: $(LIB) $(LIB_DIR)/rmana.o analyzer.o $(MODULES)
# 	$(CXX) $(CFLAGS) -o $@ $(LIB_DIR)/rmana.o analyzer.o $(MODULES) \
# 	$(LIB) $(LDFLAGS) $(ROOTLIBS) $(LIBS)

# %.o: %.c experim.h
# 	$(CXX) $(USERFLAGS) $(ROOTCFLAGS) $(CFLAGS) $(OSFLAGS) -o $@ -c $<

clean::
	rm -f *.o *~ \#*
	rm frontend

#end file
