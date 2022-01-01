CFLAGS=-std=c++2a -g

INCLUDES := $(wildcard *.h)

pontacc: main.o
	$(CXX) $(CFLAGS) -o pontacc main.o $(LDFLAGS)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) -o $@ -c $<

test: pontacc
	./test.sh

clean:
	rm -f pontacc *.o *~ tmp*

.PHONY: test clean
