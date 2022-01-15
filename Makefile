CFLAGS=-std=c++2a -g

INCLUDES := $(wildcard *.h)
CPP_FILES := $(wildcard *.cpp)

OBJS := $(patsubst %.cpp, %.o, $(CPP_FILES))

pontacc: $(OBJS)
	$(CXX) $(CFLAGS) -o pontacc $(OBJS) $(LDFLAGS)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) -Wall -Wextra -Wno-sign-compare -Wno-unused-function -o $@ -c $<

test: pontacc
	./test.sh

clean:
	rm -f pontacc *.o *~ tmp*

.PHONY: test clean
