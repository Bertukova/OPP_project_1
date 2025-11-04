.PHONY: all build test clean

SRC_DIR = src
TEST_DIR = tests
GTEST_DIR = googletest

all: build

build:
	mkdir -p build
	g++ -std=c++17 -I./include $(SRC_DIR)/*.cpp -o build/app

test:
	mkdir -p build
	g++ -std=c++17 -I./$(GTEST_DIR)/googletest/include \
		$(TEST_DIR)/test.cpp \
		-L./$(GTEST_DIR)/build/lib -lgtest -lgtest_main -lpthread \
		-o build/tests.exe
	cd build && ./tests.exe

clean:
	rm -rf build
