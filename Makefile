CFLAGS=-std=c++2a -g

pontacc: main.o
	$(CXX) $(CFLAGS) -o pontacc main.o $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -o $@ -c $<

test: pontacc
	./test.sh

clean:
	rm -f pontacc *.o *~ tmp*

.PHONY: test clean
