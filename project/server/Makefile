# 目录结构
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
EXT_DIR = ..

# 编译器
CC = gcc
CFLAGS = -Wall -g -I. -I$(EXT_DIR)
LDFLAGS = -lsqlite3

# 目标文件
TARGET = $(BIN_DIR)/server

# 源文件
SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/socket.c \
       $(SRC_DIR)/epoll_set.c \
       $(SRC_DIR)/db.c \
       $(SRC_DIR)/handle_message.c \
       $(SRC_DIR)/send.c \
       $(EXT_DIR)/cJSON.c

# 目标文件列表
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter %/main.c %/socket.c %/epoll_set.c %/db.c %/handle_message.c %/send.c,$(SRCS))) \
       $(OBJ_DIR)/cJSON.o

# 头文件依赖
DEPS = $(SRCS:%.c=%.d)

# 默认目标
all: $(TARGET)

# 链接生成可执行文件
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# 编译源文件为目标文件
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/cJSON.o: $(EXT_DIR)/cJSON.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 头文件依赖规则
-include $(DEPS)

# 运行
run: $(TARGET)
	./$(TARGET)

# 清理
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) my.db

# 重新编译
rebuild: clean all

.PHONY: all clean run rebuild
