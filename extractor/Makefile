OBJS = oclFeatureExt.o
CC = g++
DEBUG = -g -DDEBUG

CPPFLAGS = -Wall -c `llvm-config --cppflags` -ljsoncpp -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_NO_CXX11_SCOPED_ENUMS -lboost_system -lboost_filesystem -std=c++11 ${DEBUG}
LDFLAGS = -Wall `llvm-config --ldflags`
LIBS = `llvm-config --libs --system-libs`


EXEC = oclFeatureExt.out

all : $(EXEC)

$(EXEC) : $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS) $(LIBS) -lboost_filesystem 

$(OBJS) : %.o : %.cpp
	$(CC) $(CPPFLAGS) $< -o $@

clean:
		rm -f *.o *~ $(EXEC)
