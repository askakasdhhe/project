CC = gcc
CFLAGS = -Wall -g -Iinclude -I..
LDFLAGS =

SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
BIN_DIR = bin
EXT_DIR = ..

TARGET = $(BIN_DIR)/client

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/client_net.c \
       $(SRC_DIR)/choice.c \
       $(SRC_DIR)/device_register.c \
       $(SRC_DIR)/device_login.c \
       $(SRC_DIR)/device_send_data.c \
       $(SRC_DIR)/device_menu.c \
       $(SRC_DIR)/admin_login.c \
       $(SRC_DIR)/admin_menu.c \
       $(SRC_DIR)/admin_query_history.c \
       $(EXT_DIR)/cJSON.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(filter %/main.c %/client_net.c %/choice.c %/device_register.c %/device_login.c %/device_send_data.c %/device_menu.c %/admin_login.c %/admin_menu.c %/admin_query_history.c,$(SRCS))) \
       $(OBJ_DIR)/cJSON.o

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/cJSON.o: $(EXT_DIR)/cJSON.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run
