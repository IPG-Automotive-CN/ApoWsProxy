#******************************************************************************
#**  CarMaker - Version 13.1
#**  Vehicle Dynamic Simulation Toolkit
#**
#**  Copyright (C)   IPG Automotive GmbH
#**                  Bannwaldallee 60             Phone  +49.721.98520.0
#**                  76185 Karlsruhe              Fax    +49.721.98520.99
#**                  Germany                      WWW    www.ipg-automotive.com
#******************************************************************************

# include C:/IPG/carmaker/win64-13.1/include/MakeDefs.win64
include /opt/ipg/carmaker/linux64-13.1/include/MakeDefs.linux64


#OPT_CFLAGS =	-g
OPT_CFLAGS =	-Os -DNDEBUG


LIBS =		$(CARMAKER_LIB_DIR)/libapo-client-$(ARCH).a $(CARMAKER_LIB_DIR)/libipgroad.a $(CARMAKER_LIB_DIR)/libcarmaker.a

URI_LIB =		$(CARMAKER_LIB_DIR)/liburiparser.a
Z_LIB =			$(CARMAKER_LIB_DIR)/libz-linux64.a
LD_LIBS_OS =  $(Z_LIB) $(URI_LIB) -lm  -ldl -lusb-1.0 -lpthread -lrt


OBJS =		ApoClntDemo.o ApoClnt.o GuiCmd.o DVAWrite.o DVARead.o WSServer.o TestrunMgr.o \
			UAQMgr.o

CXXFLAGS += -Wall -g -pthread -std=c++17 

all: 		ApoClntDemo.$(ARCH)$(EXE_EXT)

ifeq ($(ARCH), win64)
OBJS +=		resources.o
resources.o:	resources.rc ApoClntDemo.exe.manifest
	$(WINDRES) -O coff -o $@ resources.rc
endif

ApoClntDemo.$(ARCH)$(EXE_EXT):	$(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LIBS) $(LD_LIBS_OS) -lwebsockets

clean:
	rm -f *~ *.o a.out a.exe core core.* ApoClntDemo.$(ARCH)$(EXE_EXT)



ApoClntDemo.o:	ApoClnt.h DVA.h
ApoClnt.o:	ApoClnt.h
GuiCmd.o:	GuiCmd.h ApoClnt.h
DVARead.o:	DVA.h ApoClnt.h
DVAWrite.o:	DVA.h

