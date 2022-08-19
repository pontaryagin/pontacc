CFLAGS=-std=c++2a -g -O0

INCLUDES := $(wildcard *.h)
CPP_FILES := $(wildcard *.cpp)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(TEST_SRCS:.c=.exe)

OBJS := $(patsubst %.cpp, %.o, $(CPP_FILES))

pontacc: $(OBJS)
	$(CXX) $(CFLAGS) -o pontacc $(OBJS) $(LDFLAGS)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) -Wall -Wextra -Wno-sign-compare -Wno-unused-function -o $@ -c $<

test/%.exe: pontacc test/%.c
	$(CC) -o- -E -P -C test/$*.c > test/$*.c.tmp
	./pontacc -o test/$*.s test/$*.c.tmp
	$(CC) -S -o $@.s test/$*.s -xc test/common
	$(CC) -o $@ test/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf chibicc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean
