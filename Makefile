CFLAGS=-std=c++2a -g -O0

INCLUDES := $(wildcard *.h)
CPP_FILES := $(wildcard *.cpp)

TEST_SRCS=$(wildcard test/*.c)
TESTS=$(patsubst test/%.c, test/.build/%.exe, $(TEST_SRCS))
TEST_BUILD_DATA=test/.build

OBJS := $(patsubst %.cpp, %.o, $(CPP_FILES))

pontacc: $(OBJS)
	$(CXX) $(CFLAGS) -o pontacc $(OBJS) $(LDFLAGS)

%.o: %.cpp $(INCLUDES)
	$(CXX) $(CFLAGS) -Wall -Wextra -Wno-sign-compare -Wno-unused-function -o $@ -c $<

test/.build/%.exe: pontacc test/%.c
	$(CC) -o- -E -P -C test/$*.c > $(TEST_BUILD_DATA)/$*.c.preprocessed
	./pontacc -o $(TEST_BUILD_DATA)/$*.s $(TEST_BUILD_DATA)/$*.c.preprocessed
	$(CC) -g -O0 -S -o $@.s $(TEST_BUILD_DATA)/$*.s -xc test/common
	$(CC) -o $@ $(TEST_BUILD_DATA)/$*.s -xc test/common

test: $(TESTS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -rf chibicc tmp* $(TESTS) test/*.s test/*.exe
	find * -type f '(' -name '*~' -o -name '*.o' ')' -exec rm {} ';'

.PHONY: test clean
