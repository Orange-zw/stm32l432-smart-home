# Makefile for Embedded-List
# 支持编译库文件和测试程序

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
INCLUDES = -I.

# 库文件
LIB_SOURCES = embedded_list.c list_save.c
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIB_NAME = libembedded_list.a

# 测试文件
TEST_SOURCES = test_main.c test_list.c
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
TEST_TARGET = test_main

# 默认目标：编译库和测试
all: lib test

# 编译静态库
lib: $(LIB_NAME)

$(LIB_NAME): $(LIB_OBJECTS)
	ar rcs $@ $^
	@echo "Library built: $(LIB_NAME)"

# 编译测试程序
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJECTS) $(LIB_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(INCLUDES)
	@echo "Test program built: $(TEST_TARGET)"

# 编译对象文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@ $(INCLUDES)

# 运行测试
run-test: test
	./$(TEST_TARGET)

# 清理编译产物
clean:
	rm -f $(LIB_OBJECTS) $(TEST_OBJECTS) $(LIB_NAME) $(TEST_TARGET)
	rm -f *.exe *.o *.a
	rm -f test_list_persist.bin

# 安装（复制到系统目录，可选）
PREFIX ?= /usr/local
install: lib
	@mkdir -p $(PREFIX)/lib $(PREFIX)/include
	cp $(LIB_NAME) $(PREFIX)/lib/
	cp embedded_list.h list_save.h $(PREFIX)/include/
	@echo "Library installed to $(PREFIX)"

# 卸载
uninstall:
	rm -f $(PREFIX)/lib/$(LIB_NAME)
	rm -f $(PREFIX)/include/embedded_list.h $(PREFIX)/include/list_save.h
	@echo "Library uninstalled"

.PHONY: all lib test run-test clean install uninstall

