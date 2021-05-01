CC = gcc
CFLAGS = -g -O2 -Wall -Werror
CXX = g++ -std=c++11
CXXFLAGS = -g -O3 -Wall -Wextra -Werror

BUILD_DIR = ./build
TARGET_EXEC = $(BUILD_DIR)/proxy

top_srcdir = .

LIB_DIR = .
SRC_DIR = ./src
INC_DIRS = \
  $(SRC_DIR) \
  $(LIB_DIR)/open-nFAPI/nfapi/public_inc \
  $(LIB_DIR)/open-nFAPI/pnf_sim/inc \
  $(LIB_DIR)/open-nFAPI/nfapi/inc \
  $(LIB_DIR)/open-nFAPI/pnf/public_inc \
  $(LIB_DIR)/open-nFAPI/pnf/inc \
  $(LIB_DIR)/open-nFAPI/common/public_inc \
  $(LIB_DIR)/open-nFAPI/common/inc \

LIB_SOURCES = \
  $(LIB_DIR)/open-nFAPI/nfapi/src/nfapi.c \
  $(LIB_DIR)/open-nFAPI/nfapi/src/nfapi_p4.c \
  $(LIB_DIR)/open-nFAPI/nfapi/src/nfapi_p5.c \
  $(LIB_DIR)/open-nFAPI/nfapi/src/nfapi_p7.c \
  $(LIB_DIR)/open-nFAPI/pnf/src/pnf_interface.c \
  $(LIB_DIR)/open-nFAPI/pnf/src/pnf_p7_interface.c \
  $(LIB_DIR)/open-nFAPI/pnf/src/pnf.c \
  $(LIB_DIR)/open-nFAPI/pnf/src/pnf_p7.c \
  $(LIB_DIR)/open-nFAPI/common/src/debug.c \

SRCS = \
	$(SRC_DIR)/proxy.cc \
	$(SRC_DIR)/lte_proxy.cc \
	$(SRC_DIR)/nr_proxy.cc \
	$(SRC_DIR)/nfapiutils.c \
	$(SRC_DIR)/nfapi_pnf.c \
	$(SRC_DIR)/queue.c \
	$(LIB_SOURCES)

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)

INC_FLAGS := $(addprefix -I,$(INC_DIRS))

LDFLAGS = -lasan -pthread  -lpthread -lz -lsctp 

MKDIR_P ?= mkdir -p

$(TARGET_EXEC): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(INC_FLAGS) $(CFLAGS) -c $< -o $@

# c++ source
$(BUILD_DIR)/%.cc.o: %.cc
	$(MKDIR_P) $(dir $@)
	$(CXX) $(INC_FLAGS) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)

-include $(DEPS)

# Run static analysis checks on Python scripts
.PHONY: mypy
mypy:
	python3 -m mypy proxy_testscript.py
