CC:= gcc
CCFLAGS:= -I./include
CXX:= g++
CXXFLAGS:= -I./include --std=c++17 -fPIC -fpermissive
LD:= g++
LDFLAGS:= $(CXXFLAGS)
SRC_PATH:= src/
BUILD_PATH:= build/
SHARED_OBJS:= $(addprefix $(BUILD_PATH), shell.o command.o util.o process_manager.o signal_handler.o socket_server.o user_manager.o)
EXECS:= npshell np_simple np_simple_proc

.PHONY: all run clean debug

all: $(EXECS)

debug: CXXFLAGS += -DDEBUG -g
debug: CCFLAGS += -DDEBUG -g
debug: $(EXECS)

clean:
	-rm $(SHARED_OBJS) $(addprefix $(BUILD_PATH)entry_, $(addsuffix .o, $(EXECS))) $(EXECS) 2>/dev/null

$(BUILD_PATH)%.o: $(SRC_PATH)%.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(EXECS): %: $(BUILD_PATH)entry_%.o $(SHARED_OBJS)
	$(LD) $(LDFLAGS) -o $@ $< $(SHARED_OBJS)