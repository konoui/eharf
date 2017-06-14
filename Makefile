CXX = g++

PROG := main.out
SRCS := main.o
SRCS += eh_frame.o dwarf4.o
SRCS += ia64_cxx_abi.o
SRCS += registers_intel_x64.o
OBJS := $(SRCS:%.c=%.o)

CXXFLAGS := -g -Wall -gdwarf-4 -std=gnu++11
CXXFLAGS += -DLOG #-DOBJDUMP
CXXFLAGS += -DALERT #-DDEBUG

all: $(PROG)
$(PROG): $(OBJS)
	$(CXX) $^ -o $@

%.o: %.c
	$(CXX) $(CXXFLAGS) $(SRCS) -c

clean:
	rm -f $(PROG) $(OBJS)

