TARGET_EXEC := sievecli

BUILD_DIR := ./target
SRC_DIRS := ./src

SRCS := $(shell find $(SRC_DIRS) -name '*.c')
OBJS := $(SRCS:%.c=$(BUILD_DIR)/%.o)

INC_DIRS := ./include
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CC = gcc
CFLAGS = -g3 -Wall -Wextra -Werror -pedantic

$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)