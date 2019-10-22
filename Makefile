CC:= gcc
CCFLAGS:= -I./include
CXX:= g++
CXXFLAGS:= -I./include --std=c++14 -fPIC -fpermissive
LD:= g++
LDFLAGS:= $(CXXFLAGS)
OBJS:= main.o shell.o command.o util.o process_manager.o signal_handler.o
EXEC:= npshell

.PHONY: all run clean debug

all: $(EXEC)

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: $(EXEC)

run: $(EXEC)
	-./$(EXEC)

clean:
	-rm $(OBJS) $(EXEC)

%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXEC): $(OBJS)
	$(LD) $(LDFLAGS) -o $(EXEC) $(OBJS)