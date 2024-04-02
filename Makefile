.PHONY: all build test clean

CC = gcc
SOURCES = $(wildcard src/*.c)
TEST = true
BUILD_FLAGS = \
	-std=gnu11 -pedantic -Wall \
	-Wno-missing-braces -Wextra -Wno-missing-field-initializers -Wformat=2 \
	-Wswitch-default -Wswitch-enum -Wcast-align -Wpointer-arith \
	-Wbad-function-cast -Wstrict-overflow=5 -Wstrict-prototypes -Winline \
	-Wundef -Wnested-externs -Wcast-qual -Wshadow -Wunreachable-code \
	-Wlogical-op -Wfloat-equal -Wstrict-aliasing=2 -Wredundant-decls \
	-Wold-style-definition -Wno-pedantic-ms-format -Werror \
	-g -O0 \
	-fno-omit-frame-pointer -ffloat-store -fno-common

ifeq ($(OS),Windows_NT)
	NULL_CMD = cd.
	LINK_FLAGS = 
	BUILD_SHARED_OUT = bin/channel.dll
	MOVE_OBJECTS = move /y *.o bin >NUL
	CLEAN_OBJECTS = del bin\*.o
	TEST_BINARY = bin\test
	POST_BUILD_CMD = $(NULL_CMD)
	CLEAN_CMD = del bin\libchannel.so bin\channel.dll bin\test bin\test.exe bin\*.o *.o
else
	NULL_CMD = :
	LINK_FLAGS = -lpthread
	BUILD_SHARED_OUT = bin/libchannel.so
	MOVE_OBJECTS = mv *.o bin/
	CLEAN_OBJECTS = rm -f bin/*.o
	TEST_BINARY = ./bin/test
	POST_BUILD_CMD = chmod +x ./bin/test
	CLEAN_CMD = rm -f bin/libchannel.so bin/channel.dll bin/test bin/test.exe bin/*.o *.o
endif

ifeq ($(TEST),true)
	BUILD_DIRECTIVES = -DCHANNEL_TEST
	BUILD_TEST_BINARY_CMD = \
		$(CC) -o bin/test \
		$(BUILD_FLAGS) \
		test/*.c -L./bin -Wl,-rpath=./bin -lchannel -lpthread
else
	BUILD_DIRECTIVES =
	BUILD_TEST_BINARY_CMD = $(NULL_CMD)
endif

all: build

build:
	$(CC) -c \
		-fPIC \
		$(BUILD_FLAGS) \
		$(BUILD_DIRECTIVES) \
		$(SOURCES) && \
	$(MOVE_OBJECTS) && \
	$(CC) -o $(BUILD_SHARED_OUT) \
		-fPIC \
		-shared \
		$(BUILD_FLAGS) \
		bin/*.o \
		$(LINK_FLAGS) && \
	$(BUILD_TEST_BINARY_CMD) && \
	$(POST_BUILD_CMD) && \
	$(CLEAN_OBJECTS)

test:
	$(TEST_BINARY)

clean:
	$(CLEAN_CMD)
